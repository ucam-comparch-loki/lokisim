/*
 * MemoryBank.cpp
 *
 * One bank of the banked L1/L2 cache system. Most of the logic has been
 * farmed out to the files in Operations/ so this class mostly ensures that the
 * required data is available.
 *
 *  Created on: 1 Jul 2015
 *      Author: db434
 */

#include <iostream>
#include <iomanip>

using namespace std;

#include "MemoryBank.h"
#include "MainMemory.h"
#include "Operations/MemoryOperationDecode.h"
#include "../ComputeTile.h"
#include "../../Chip.h"
#include "../../Network/Topologies/LocalNetwork.h"
#include "../../Utility/Arguments.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/MemoryBank.h"
#include "../../Utility/Instrumentation/Network.h"
#include "../../Utility/Parameters.h"
#include "../../Utility/Warnings.h"
#include "../../Exceptions/ReadOnlyException.h"
#include "../../Exceptions/UnsupportedFeatureException.h"

class ComputeTile;

// The latency of accessing a memory bank, aside from the cost of accessing the
// SRAM. This typically includes the network to/from the bank.
#define EXTERNAL_LATENCY 1

// Additional latency to apply to get the total memory latency to match the
// MEMORY_BANK_LATENCY parameter. Includes one extra cycle of unavoidable
// latency within the bank.
#define INTERNAL_LATENCY (MEMORY_BANK_LATENCY - EXTERNAL_LATENCY - 1)


uint log2(uint value) {
  assert(value > 1);

  uint result = 0;
  uint temp = value >> 1;

  while (temp != 0) {
    result++;
    temp >>= 1;
  }

  assert(1UL << result == value);

  return result;
}

// Compute the position in SRAM that the given memory address is to be found.
// Memory address contains:
// | tag | index | bank | offset |
//  * offset = 5 bits (32 byte cache line)
//  * index + offset = log2(bytes in bank) bits
//  * bank = up to 3 bits used to choose which bank to access
//  * tag = any bits remaining
//
// Since the number of bank bits is variable, but we don't want to move the
// index bits around or change the size of the tag field, we hash the maximum
// number of bank bits in with a fixed-position index field. Note that these
// overlapping bits must now also be included in the tag.
//
// Note that this can result in a deterministic but counter-intuitive mapping
// of addresses in scratchpad mode.
SRAMAddress MemoryBank::getPosition(MemoryAddr address, MemoryAccessMode mode) const {
  // Slight hack: the contains() method is where one might expect a tag check
  // to happen, but I use that method frequently to help with assertions. I
  // instead perform the instrumentation here, as this method will be executed
  // exactly once per operation.
  if (mode == MEMORY_CACHE)
    Instrumentation::MemoryBank::checkTags(id.globalMemoryNumber(), address);

  static const uint indexBits = log2(CACHE_LINES_PER_BANK);
  uint offset = getOffset(address);
  uint bank = (address >> 5) & 0x7;
  uint index = (address >> 8) & ((1 << indexBits) - 1);
  uint slot = index ^ (bank << (indexBits - 3)); // Hash bank into the upper bits
  return (slot << 5) | offset;
}

// Return the position in the memory address space of the data stored at the
// given position.
MemoryAddr MemoryBank::getAddress(SRAMAddress position) const {
  return mTags[getLine(position)] + getOffset(position);
}

// Return whether data from `address` can be found at `position` in the SRAM.
bool MemoryBank::contains(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) const {
  switch (mode) {
    case MEMORY_CACHE: {
      uint cacheLine = getLine(position);
      return (mTags[cacheLine] == getTag(address)) && mValid[cacheLine];
    }
    case MEMORY_SCRATCHPAD:
      return true;
    default:
      loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
      return false;
  }
}

// Ensure that a valid copy of data from `address` can be found at `position`.
void MemoryBank::allocate(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) {
  switch (mode) {
    case MEMORY_CACHE:
      if (!contains(address, position, mode)) {
        LOKI_LOG << this->name() << " cache miss at address " << LOKI_HEX(address) << endl;
        Instrumentation::MemoryBank::replaceCacheLine(id.globalMemoryNumber(), mValid[getLine(position)], mDirty[getLine(position)]);

        // Send a request for the missing cache line.
        NetworkRequest readRequest(getTag(address), id, FETCH_LINE, true);
        MemoryMetadata metadata = readRequest.getMemoryMetadata();
        metadata.skipL2 = mActiveRequest->getMetadata().skipL2;
        readRequest.setMetadata(metadata.flatten());
        sendRequest(readRequest);

        // Stop serving the request until allocation is complete.
        mState = STATE_ALLOCATE;
      }
      break;
    case MEMORY_SCRATCHPAD:
      break;
  }
}

// Ensure that there is a space to write data to `address` at `position`.
void MemoryBank::validate(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) {
  switch (mode) {
    case MEMORY_CACHE: {
      if (mTags[getLine(position)] != getTag(address))
        flush(position, mode);
      mTags[getLine(position)] = getTag(address);
      mValid[getLine(position)] = true;

      // Hack. It might not always be the active request doing this?
      mL2Skip[getLine(position)] = mActiveRequest->getMetadata().skipL2;

      // Some extra bookkeeping to help with debugging.
      bool inMainMemory = chip()->backedByMainMemory(id.tile, address);
      if (inMainMemory) {
        MemoryAddr globalAddress = chip()->getAddressTranslation(id.tile, address);
        mMainMemory->claimCacheLine(id, globalAddress);
      }

      break;
    }

    case MEMORY_SCRATCHPAD:
      break;
  }
}

// Invalidate the cache line which contains `position`.
void MemoryBank::invalidate(SRAMAddress position, MemoryAccessMode mode) {
  switch (mode) {
    case MEMORY_CACHE:
      mValid[getLine(position)] = false;
      break;
    case MEMORY_SCRATCHPAD:
      break;
  }
}

// Flush the cache line containing `position` down the memory hierarchy, if
// necessary. The line is not invalidated, but is no longer dirty.
void MemoryBank::flush(SRAMAddress position, MemoryAccessMode mode) {
  switch (mode) {
    case MEMORY_CACHE:
      if (mValid[getLine(position)] && mDirty[getLine(position)]) {
        // Send a header flit telling where this line should be stored.
        NetworkRequest header(mTags[getLine(position)], id, STORE_LINE, false);
        MemoryMetadata metadata = header.getMemoryMetadata();
        metadata.skipL2 = mL2Skip[getLine(position)];
        header.setMetadata(metadata.flatten());
        sendRequest(header);

        if (ENERGY_TRACE)
          Instrumentation::MemoryBank::startOperation(id.globalMemoryNumber(),
              FLUSH_LINE, mTags[getLine(position)], false, ChannelID(id,0));

        // The flush state handles sending the line itself.
        mPreviousState = mState;
        mState = STATE_FLUSH;
        mCacheLineCursor = 0;
      }
      else
        mState = STATE_REQUEST;

      mDirty[getLine(position)] = false;
      break;
    case MEMORY_SCRATCHPAD:
      break;
  }
}

// Return whether a payload flit is available. `level` tells whether this bank
// is being treated as an L1 or L2 cache.
bool MemoryBank::payloadAvailable(MemoryLevel level) const {
  if (mReadFromMissBuffer) {
    return !mMissBuffer.empty();
  }
  else {
    switch (level) {
      case MEMORY_L1:
        return !mInputQueue.empty() && isPayload(mInputQueue.peek());
      case MEMORY_L2:
        return requestSig.valid() && isPayload(requestSig.read());
      default:
        loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
        return false;
    }
  }
}

// Retrieve a payload flit. `level` tells whether this bank is being treated
// as an L1 or L2 cache.
uint32_t MemoryBank::getPayload(MemoryLevel level) {
  assert(payloadAvailable(level));

  NetworkRequest request;

  if (mReadFromMissBuffer) {
    request = mMissBuffer.read();
  }
  else {
    switch (level) {
      case MEMORY_L1:
        request = mInputQueue.read();
        LOKI_LOG << this->name() << " dequeued " << request << endl;
        break;
      case MEMORY_L2: {
        request = requestSig.read();
        requestSig.ack();
        break;
      }
      default:
        loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
        break;
    }
  }

  loki_assert(isPayload(request));
  return request.payload().toUInt();
}

// Make a load-linked reservation.
void MemoryBank::makeReservation(ComponentID requester, MemoryAddr address) {
  mReservations.makeReservation(requester, address);
}

// Return whether a load-linked reservation is still valid.
bool MemoryBank::checkReservation(ComponentID requester, MemoryAddr address) const {
  return mReservations.checkReservation(requester, address);
}

void MemoryBank::writeWord(SRAMAddress position, uint32_t data) {
  checkAlignment(position, BYTES_PER_WORD);

  // A bit of a hack to get some extra information about the request.
  MemoryAddr address;
  bool scratchpad = false;
  if (mActiveRequest == NULL)
    address = getAddress(position);
  else {
    address = mActiveRequest->getAddress();
    scratchpad = mActiveRequest->getAccessMode() == MEMORY_SCRATCHPAD;
  }

  mData[position/BYTES_PER_WORD] = data;
  if (!scratchpad)
    mDirty[getLine(position)] = true;

  mReservations.clearReservation(address);
}

void MemoryBank::processIdle() {
  loki_assert_with_message(mState == STATE_IDLE, "State = %d", mState);

  // Refills have priority because other requests may depend on them.
  if (responseAvailable()) {
    mState = STATE_REFILL;
    mCacheLineCursor = 0;
    next_trigger(sc_core::SC_ZERO_TIME);
  }
  // Check for any other requests.
  else if (requestAvailable()) {
    // If the hit-under-miss option is enabled and we are currently serving a
    // miss, peek the next request in the queue to see if it is a hit.

    mActiveRequest = peekRequest();

    if (MEMORY_HIT_UNDER_MISS && (mMissingRequest != NULL)) {
      // Don't reorder data being sent to the same channel.
      if (mMissingRequest->getDestination() == mActiveRequest->getDestination())
        return;

      // Don't start a request for the same location as the missing request.
      if (getTag(mMissingRequest->getAddress()) == getTag(mActiveRequest->getAddress()))
        return;

      // Don't start another request if it will miss.
      if (!mActiveRequest->inCache())
        return;

      // Don't start a request which skips this cache, but will return data
      // through it.
      if (mActiveRequest->needsForwarding() && mActiveRequest->resultsToSend())
        return;

      LOKI_LOG << this->name() << " starting hit-under-miss" << endl;
    }

    consumeRequest(mActiveRequest->getMemoryLevel());
    mState = STATE_REQUEST;
    next_trigger(sc_core::SC_ZERO_TIME);

    LOKI_LOG << this->name() << " starting " << memoryOpName(mActiveRequest->getMetadata().opcode)
        << " request from component " << mActiveRequest->getDestination().component << endl;
  }
  // Nothing to do - wait for input to arrive.
  else {
    next_trigger(responseAvailableEvent() | requestAvailableEvent());
  }
}

void MemoryBank::processRequest() {
  loki_assert_with_message(mState == STATE_REQUEST, "State = %d", mState);
  loki_assert(mActiveRequest != NULL);

  // Before we even consider serving a request, we must make sure that there
  // is space to buffer any potential results.

  if (!canSendRequest()) {
    LOKI_LOG << this->name() << " delayed request due to full output request queue" << endl;
    next_trigger(canSendRequestEvent());
  }
  else if (mActiveRequest->needsForwarding()) {
    forwardRequest(mActiveRequest->getOriginal());
    if (mActiveRequest->awaitingPayload())
      mState = STATE_FORWARD;
    else {
      assert(mMissingRequest == NULL);
      mMissingRequest = mActiveRequest;
      finishedRequestForNow();
    }
  }
  else if (mActiveRequest->resultsToSend() && !canSendResponse(mActiveRequest->getMemoryLevel())) {
    LOKI_LOG << this->name() << " delayed request due to full output queue" << endl;
    next_trigger(canSendResponseEvent(mActiveRequest->getMemoryLevel()));
  }
  else if (!iClock.negedge()) {
    next_trigger(iClock.negedge_event());
  }
  else if (!mActiveRequest->preconditionsMet()) {
    mActiveRequest->prepare();
  }
  else if (!mActiveRequest->complete()) {
    mActiveRequest->execute();
  }

  // If the operation has finished, and we haven't moved to some other state
  // for further processing, end the request and prepare for a new one.
  if (mActiveRequest != NULL && mActiveRequest->complete() && mState == STATE_REQUEST) {
    finishedRequest();
    mReadFromMissBuffer = false;
  }
}

void MemoryBank::processAllocate() {
  loki_assert_with_message(mState == STATE_ALLOCATE, "State = %d", mState);
  loki_assert(mActiveRequest != NULL);

  // Use inCache to check whether the line has already been allocated. The data
  // won't have arrived yet.
  if (!mActiveRequest->inCache()) {
    // The request has already called allocate() above, so data for the new line
    // is on its way. Now we must prepare the line for the data's arrival.
    mActiveRequest->validateLine();

    // Put the request to one side until its data comes back.
    mMissingRequest = mActiveRequest;
    mCacheMissEvent.notify(sc_core::SC_ZERO_TIME);

    if (mMissingRequest->awaitingPayload())
      mCopyToMissBuffer = true;

    if (mState != STATE_FLUSH) {
      mState = STATE_IDLE;
      mActiveRequest = NULL;
    }
  }
  else {
    mState = STATE_IDLE;
    mActiveRequest = NULL;
  }
}

void MemoryBank::processFlush() {
  loki_assert_with_message(mState == STATE_FLUSH, "State = %d", mState);
  loki_assert(mActiveRequest != NULL);
  // It is assumed that the header flit has already been sent. All that is left
  // is to send the cache line.

  if (!iClock.negedge())
    next_trigger(iClock.negedge_event());
  else if (canSendRequest()) {
    SRAMAddress position = getTag(mActiveRequest->getSRAMAddress()) + mCacheLineCursor;
    uint32_t data = readWord(position);

    Instrumentation::MemoryBank::continueOperation(id.globalMemoryNumber(),
        FLUSH_LINE, mTags[getLine(position)] + mCacheLineCursor, false, ChannelID(id,0));

    mCacheLineCursor += BYTES_PER_WORD;

    // Flush has finished if the cursor has covered a whole cache line.
    bool endOfPacket = mCacheLineCursor >= CACHE_LINE_BYTES;
    if (endOfPacket)
      mState = mPreviousState;

    NetworkRequest flit(data, id, PAYLOAD, endOfPacket);
    sendRequest(flit);
  }
  else
    next_trigger(canSendRequestEvent());
}

void MemoryBank::processRefill() {
  loki_assert_with_message(mState == STATE_REFILL, "State = %d", mState);
  loki_assert(mMissingRequest != NULL);

  if (!iClock.negedge())
    next_trigger(iClock.negedge_event());
  else if (responseAvailable()) {

    // Don't store data locally if the request bypasses this cache.
    if (mMissingRequest->needsForwarding()) {
      if (canSendResponse(mMissingRequest->getMemoryLevel())) {
        uint32_t data = getResponse();
        mMissingRequest->sendResult(data);

        if (mMissingRequest->resultsToSend()) {
          next_trigger(responseAvailableEvent());
        }
        else {
          mState = STATE_IDLE;
          mMissingRequest = NULL;
        }
      }
      else
        next_trigger(canSendResponseEvent(mMissingRequest->getMemoryLevel()));
    }
    else {
      uint32_t data = getResponse();
      SRAMAddress position = getTag(mMissingRequest->getSRAMAddress()) + mCacheLineCursor;
      writeWord(position, data);

      mCacheLineCursor += BYTES_PER_WORD;

      // Refill has finished if the cursor has covered a whole cache line.
      if (mCacheLineCursor >= CACHE_LINE_BYTES) {
        mState = STATE_REQUEST;
        mActiveRequest = mMissingRequest;
        mMissingRequest = NULL;
        mReadFromMissBuffer = true;

        // Storing the requested words will have dirtied the cache line, but it
        // is actually clean.
        mDirty[getLine(position)] = false;

        LOKI_LOG << this->name() << " resuming " << memoryOpName(mActiveRequest->getMetadata().opcode) << " request" << endl;
      }
      else
        next_trigger(responseAvailableEvent());
    }
  }
  else
    next_trigger(responseAvailableEvent());
}

void MemoryBank::processForward() {
  loki_assert_with_message(mState == STATE_FORWARD, "State = %d", mState);
  loki_assert(mActiveRequest != NULL);

  // Before we even consider serving a request, we must make sure that there
  // is space to buffer any potential results.

  if (!canSendRequest()) {
    LOKI_LOG << this->name() << " delayed request due to full output request queue" << endl;
    next_trigger(canSendRequestEvent());
  }
  else if (!payloadAvailable(mActiveRequest->getMemoryLevel())) {
    next_trigger(requestAvailableEvent());
  }
  else {
    NetworkRequest payload;
    switch (mActiveRequest->getMemoryLevel()) {
      case MEMORY_L1:
        payload = mInputQueue.read();
        LOKI_LOG << this->name() << " dequeued " << payload << endl;
        break;
      case MEMORY_L2:
        payload = requestSig.read();
        requestSig.ack();
        break;
      default:
        loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
        break;
    }

    forwardRequest(payload);

    if (payload.getMetadata().endOfPacket) {
      // Treat a forwarded request as a missing request if we need to wait for
      // results - we need to preserve all state until the response has been
      // passed back to the original requester.
      if (mActiveRequest->resultsToSend()) {
        assert(mMissingRequest == NULL);
        mMissingRequest = mActiveRequest;
        finishedRequestForNow();
      }
      else
        finishedRequest();
    }
  }
}

void MemoryBank::finishedRequest() {
  finishedRequestForNow();
  delete mActiveRequest;
}

void MemoryBank::finishedRequestForNow() {
  mState = STATE_IDLE;
  mActiveRequest = NULL;

  // Decode the next request immediately so it is ready to start next cycle.
  next_trigger(sc_core::SC_ZERO_TIME);
}

bool MemoryBank::requestAvailable() const {
  return (!mInputQueue.empty() && !isPayload(mInputQueue.peek()))
      || (requestSig.valid() && !isPayload(requestSig.read()));
}

const sc_event_or_list& MemoryBank::requestAvailableEvent() const {
  return mInputQueue.writeEvent() | requestSig.default_event();
}

MemoryOperation* MemoryBank::peekRequest() {
  loki_assert(requestAvailable());

  NetworkRequest request;
  MemoryLevel level;
  ChannelID destination;

  if (!mInputQueue.empty() && !isPayload(mInputQueue.peek())) {
    request = mInputQueue.peek();
    level = MEMORY_L1;
    destination = ChannelID(id.tile.x,
                            id.tile.y,
                            request.channelID().channel,
                            request.getMemoryMetadata().returnChannel);
  }
  else {
    loki_assert(requestSig.valid() && !isPayload(requestSig.read()));
    request = requestSig.read();
    level = MEMORY_L2;
    destination = ChannelID(request.getMemoryMetadata().returnTileX,
                            request.getMemoryMetadata().returnTileY,
                            request.getMemoryMetadata().returnChannel,
                            0);
  }

  return decodeMemoryRequest(request, *this, level, destination);
}

void MemoryBank::consumeRequest(MemoryLevel level) {
  loki_assert(requestAvailable());

  switch (level) {
    case MEMORY_L1: {
      NetworkRequest request = mInputQueue.read();
      LOKI_LOG << this->name() << " dequeued " << request << endl;
      break;
    }
    case MEMORY_L2:
      requestSig.ack();
      break;
    default:
      loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
      break;
  }
}

bool MemoryBank::canSendRequest() const {
  return !mOutputReqQueue.full();
}

const sc_event& MemoryBank::canSendRequestEvent() const {
  return mOutputReqQueue.readEvent();
}

void MemoryBank::sendRequest(NetworkRequest request) {
  loki_assert(canSendRequest());

  LOKI_LOG << this->name() << " buffering request " <<
      memoryOpName(request.getMemoryMetadata().opcode) << " " << request << endl;
  mOutputReqQueue.write(request);
}

void MemoryBank::forwardRequest(NetworkRequest request) {
  // Need to update the return address to point to this bank rather than the
  // original requester.
  NetworkRequest updated(request.payload(), id, request.getMemoryMetadata().opcode, request.getMetadata().endOfPacket);
  sendRequest(updated);
}

bool MemoryBank::responseAvailable() const {
  return iResponse.valid() && (iResponseTarget.read() == id.position-CORES_PER_TILE);
}

const sc_event& MemoryBank::responseAvailableEvent() const {
  return iResponse.default_event();
}

uint32_t MemoryBank::getResponse() {
  loki_assert(responseAvailable());
  uint32_t data = iResponse.read().payload().toUInt();
  iResponse.ack();

  LOKI_LOG << this->name() << " received response " << LOKI_HEX(data) << endl;

  return data;
}

bool MemoryBank::canSendResponse(MemoryLevel level) const {
  switch (level) {
    case MEMORY_L1:
      return !mOutputQueue.full();
    case MEMORY_L2:
      return !oResponse.valid();
    default:
      loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
      return false;
  }
}

const sc_event& MemoryBank::canSendResponseEvent(MemoryLevel level) const {
  switch (level) {
    case MEMORY_L1:
      return mOutputQueue.readEvent();
    case MEMORY_L2:
      return oResponse.ack_event();
    default:
      loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
      return mOutputQueue.readEvent();
  }
}

void MemoryBank::sendResponse(NetworkResponse response, MemoryLevel level) {
  loki_assert(canSendResponse(level));

  switch (level) {
    case MEMORY_L1:
      mOutputQueue.write(response);
      break;
    case MEMORY_L2:
      oResponse.write(response);
      break;
    default:
      loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
      break;
  }
}

void MemoryBank::copyToMissBuffer() {
  if (mCopyToMissBuffer) {
    // Check which input the missing request was reading from, and while
    // payloads arrive there, copy them into the miss buffer.
    NetworkRequest payload;
    bool dataAvailable;

    switch (mMissingRequest->getMemoryLevel()) {
      case MEMORY_L1:
        dataAvailable = !mInputQueue.empty();
        if (dataAvailable)
          payload = mInputQueue.peek();
        break;
      case MEMORY_L2:
        dataAvailable = requestSig.valid();
        if (dataAvailable)
          payload = requestSig.read();
        break;
      default:
        loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
        dataAvailable = false;
        break;
    }

    if (!dataAvailable) {
      next_trigger(requestAvailableEvent());
      return;
    }

    loki_assert(isPayload(payload));
    loki_assert(!mMissBuffer.full());
    mMissBuffer.write(payload);

    switch (mMissingRequest->getMemoryLevel()) {
      case MEMORY_L1:
        mInputQueue.read();
        break;
      case MEMORY_L2:
        requestSig.ack();
        break;
      default:
        loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
        break;
    }

    if (payload.getMetadata().endOfPacket)
      mCopyToMissBuffer = false;
    else
      next_trigger(iClock.negedge_event());
  }
}

void MemoryBank::preWriteCheck(const MemoryOperation& operation) const {
  MemoryAddr globalAddress = chip()->getAddressTranslation(id.tile, operation.getAddress());
  bool scratchpad = operation.getAccessMode() == MEMORY_SCRATCHPAD;
  bool inMainMemory = !scratchpad && chip()->backedByMainMemory(id.tile, operation.getAddress());
  if (inMainMemory && mMainMemory->readOnly(globalAddress) && WARN_READ_ONLY) {
    LOKI_WARN << this->name() << " attempting to modify read-only address" << endl;
    LOKI_WARN << "  " << operation.toString() << endl;
  }
}

void MemoryBank::processValidInput() {
  loki_assert(iData.valid());
  LOKI_LOG << this->name() << " received " << iData.read() << endl;
  loki_assert(!mInputQueue.full());
  mInputQueue.write(iData.read());
  iData.ack();
}

void MemoryBank::handleDataOutput() {
  if (mOutputQueue.empty()) {
    next_trigger(mOutputQueue.writeEvent());
  }
  else if (!localNetwork->requestGranted(id, mOutputQueue.peek().channelID())) {
    localNetwork->makeRequest(id, mOutputQueue.peek().channelID(), true);
    next_trigger(iClock.negedge_event());
  }
  else {
    NetworkResponse flit = mOutputQueue.read();
    LOKI_LOG << this->name() << " sent " << flit << endl;
    if (ENERGY_TRACE)
      Instrumentation::Network::traffic(id, flit.channelID().component);

    oData.write(flit);

    // Remove the request for network resources.
    if (flit.getMetadata().endOfPacket)
      localNetwork->makeRequest(id, flit.channelID(), false);

    next_trigger(oData.ack_event());
  }
}

// Method which sends data from the mOutputReqQueue whenever possible.
void MemoryBank::handleRequestOutput() {
  if (!iClock.posedge())
    next_trigger(iClock.posedge_event());
  else if (mOutputReqQueue.empty())
    next_trigger(mOutputReqQueue.writeEvent());
  else {
    NetworkRequest request = mOutputReqQueue.read();
    LOKI_LOG << this->name() << " sending request " <<
        memoryOpName(request.getMemoryMetadata().opcode) << " " << LOKI_HEX(request.payload()) << endl;

    loki_assert(!oRequest.valid());
    oRequest.write(request);
    next_trigger(oRequest.ack_event());
  }
}

void MemoryBank::mainLoop() {
  switch (mState) {
    case STATE_IDLE:      processIdle();      break;
    case STATE_REQUEST:   processRequest();   break;
    case STATE_ALLOCATE:  processAllocate();  break;
    case STATE_FLUSH:     processFlush();     break;
    case STATE_REFILL:    processRefill();    break;
    case STATE_FORWARD:   processForward();   break;
  }
}

void MemoryBank::updateIdle() {
  bool wasIdle = currentlyIdle;
  currentlyIdle = mState == STATE_IDLE &&
                  mInputQueue.empty() && mOutputQueue.empty() &&
                  mOutputReqQueue.empty();

  if (wasIdle != currentlyIdle)
    Instrumentation::idle(id, currentlyIdle);
}

void MemoryBank::updateReady() {
  bool ready = !mInputQueue.full();
  if (ready != oReadyForData.read())
    oReadyForData.write(ready);
}

Chip* MemoryBank::chip() const {
  return static_cast<ComputeTile*>(this->get_parent_object())->chip();
}

MemoryBank::MemoryBank(sc_module_name name, const ComponentID& ID, uint bankNumber) :
  MemoryBase(name, ID),
  mInputQueue(string(this->name()) + string(".mInputQueue"), MEMORY_BUFFER_SIZE),
  mOutputQueue("mOutputQueue", MEMORY_BUFFER_SIZE, INTERNAL_LATENCY),
  mOutputReqQueue("mOutputReqQueue", 10 /*read addr + write addr + cache line*/, 0),
  mData(MEMORY_BANK_SIZE/BYTES_PER_WORD, 0),
  mTags(CACHE_LINES_PER_BANK, 0),
  mValid(CACHE_LINES_PER_BANK, false),
  mDirty(CACHE_LINES_PER_BANK, false),
  mL2Skip(CACHE_LINES_PER_BANK, false),
  mReservations(1),
  mMissBuffer(CACHE_LINE_WORDS, "mMissBuffer"),
  mCacheMissEvent(sc_core::sc_gen_unique_name("mCacheMissEvent")),
  mL2RequestFilter("request_filter", ID, this)
{
  mActiveRequest = NULL;
  mMissingRequest = NULL;

  mState = STATE_IDLE;
  mPreviousState = STATE_IDLE;

  mCacheLineCursor = 0;

  mCopyToMissBuffer = false;
  mReadFromMissBuffer = false;

  // Magic interfaces.
  mMainMemory = NULL;
  localNetwork = NULL;

  // Connect to local components.
  oReadyForData.initialize(false);

  mL2RequestFilter.iClock(iClock);
  mL2RequestFilter.iRequest(iRequest);
  mL2RequestFilter.iRequestTarget(iRequestTarget);
  mL2RequestFilter.oRequest(requestSig);
  mL2RequestFilter.iRequestClaimed(iRequestClaimed);
  mL2RequestFilter.oClaimRequest(oClaimRequest);

  currentlyIdle = true;
  Instrumentation::idle(id, true);

  SC_METHOD(mainLoop);
  sensitive << iClock.neg();
  dont_initialize();

  SC_METHOD(updateReady);
  sensitive << mInputQueue.readEvent() << mInputQueue.writeEvent();
  // do initialise

  SC_METHOD(updateIdle);
  sensitive << mInputQueue.readEvent() << mInputQueue.writeEvent()
            << mOutputQueue.readEvent() << mOutputQueue.writeEvent()
            << mOutputReqQueue.readEvent() << mOutputReqQueue.writeEvent()
            << iResponse << oResponse;
  // do initialise

  SC_METHOD(processValidInput);
  sensitive << iData;
  dont_initialize();

  SC_METHOD(handleDataOutput);

  SC_METHOD(handleRequestOutput);

  SC_METHOD(copyToMissBuffer);
  sensitive << mCacheMissEvent;
  dont_initialize();
}

MemoryBank::~MemoryBank() {
}

void MemoryBank::setLocalNetwork(local_net_t* network) {
  localNetwork = network;
}

void MemoryBank::setBackgroundMemory(MainMemory* memory) {
  mMainMemory = memory;
}

void MemoryBank::storeData(vector<Word>& data, MemoryAddr location) {
  loki_assert(false);
}

void MemoryBank::print(MemoryAddr start, MemoryAddr end) {
  loki_assert(mMainMemory != NULL);

  if (start > end)
    std::swap(start, end);

  MemoryAddr address = (start / 4) * 4;
  MemoryAddr limit = (end / 4) * 4 + 4;

  while (address < limit) {
    uint32_t data = readWordDebug(address).toUInt();
    cout << "0x" << setprecision(8) << setfill('0') << hex << address << ":  " << "0x" << setprecision(8) << setfill('0') << hex << data << dec << endl;
    address += 4;
  }
}

Word MemoryBank::readWordDebug(MemoryAddr addr) {
  loki_assert(mMainMemory != NULL);
  loki_assert_with_message(addr % 4 == 0, "Address = 0x%x", addr);

  SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    return readWord(position);
  else
    return mMainMemory->readWord(addr);
}

Word MemoryBank::readByteDebug(MemoryAddr addr) {
  loki_assert(mMainMemory != NULL);

  SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    return readByte(position);
  else
    return mMainMemory->readByte(addr);
}

void MemoryBank::writeWordDebug(MemoryAddr addr, Word data) {
  loki_assert(mMainMemory != NULL);
  loki_assert_with_message(addr % 4 == 0, "Address = 0x%x", addr);

  SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    writeWord(position, data.toUInt());
  else
    mMainMemory->writeWord(addr, data.toUInt());
}

void MemoryBank::writeByteDebug(MemoryAddr addr, Word data) {
  loki_assert(mMainMemory != NULL);

  SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    writeByte(position, data.toUInt());
  else
    mMainMemory->writeByte(addr, data.toUInt());
}

const vector<uint32_t>& MemoryBank::dataArrayReadOnly() const {
  return mData;
}

vector<uint32_t>& MemoryBank::dataArray() {
  return mData;
}

void MemoryBank::reportStalls(ostream& os) {
  if (mInputQueue.full()) {
    os << mInputQueue.name() << " is full." << endl;
  }
  if (!mOutputQueue.empty()) {
    const NetworkResponse& outWord = mOutputQueue.peek();

    os << this->name() << " waiting to send to " << outWord.channelID() << endl;
  }
}
