//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Configurable Memory Bank Implementation
//-------------------------------------------------------------------------------------------------
// Third version of configurable memory bank model. Each memory bank is
// directly connected to the network. There are multiple memory banks per tile.
//
// Memory requests are connection-less. Instead, the new channel map table
// mechanism in the memory bank is used.
//
// The number of input and output ports is fixed to 1. The ports possess
// queues for incoming and outgoing data.
//-------------------------------------------------------------------------------------------------
// File:       MemoryBank.cpp
// Author:     Daniel Bates (Daniel.Bates@cl.cam.ac.uk)
// Created on: 08/04/2011
//-------------------------------------------------------------------------------------------------

#include <iostream>
#include <iomanip>

using namespace std;

#include "MemoryBank.h"
#include "SimplifiedOnChipScratchpad.h"
#include "Operations/MemoryOperationDecode.h"
#include "../../Datatype/Instruction.h"
#include "../../Network/Topologies/LocalNetwork.h"
#include "../../Utility/Arguments.h"
#include "../../Utility/Instrumentation/MemoryBank.h"
#include "../../Utility/Instrumentation/Network.h"
#include "../../Utility/Trace/MemoryTrace.h"
#include "../../Utility/Parameters.h"
#include "../../Utility/Warnings.h"
#include "../../Exceptions/ReadOnlyException.h"
#include "../../Exceptions/UnsupportedFeatureException.h"

// The latency of accessing a memory bank, aside from the cost of accessing the
// SRAM. This typically includes the network to/from the bank.
#define EXTERNAL_LATENCY 1

// Additional latency to apply to get the total memory latency to match the
// L1_LATENCY parameter. Includes one extra cycle of unavoidable latency within
// the bank.
#define INTERNAL_LATENCY (L1_LATENCY - EXTERNAL_LATENCY - 1)


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

MemoryAddr MemoryBank::getTag(MemoryAddr address) const {
  return address & ~0x1F;
}

SRAMAddress MemoryBank::getLine(SRAMAddress position) const {
  return position >> 5;
}

SRAMAddress MemoryBank::getOffset(SRAMAddress position) const {
  return position & 0x1F;
}

// Compute the position in SRAM that the given memory address is to be found.
SRAMAddress MemoryBank::getPosition(MemoryAddr address, MemoryAccessMode mode) const {
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

  // Slight hack: I use the contains() method frequently to help with assertions
  // so I instrument tag checks here. This method will be executed once per
  // operation.
  if (mode == MEMORY_CACHE)
    Instrumentation::MemoryBank::checkTags(id.globalMemoryNumber(), address);

  static const uint indexBits = log2(CACHE_LINES_PER_BANK);
  uint offset = getOffset(address);
  uint bank = (address >> 5) & 0x7;
  uint index = (address >> 8) & ((1 << indexBits) - 1);
  uint slot = index ^ (bank << (indexBits - 3)); // Hash bank into the upper bits
  return (slot << 5) | offset;
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
      assert(false);
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
    case MEMORY_CACHE:
      if (mTags[getLine(position)] != getTag(address))
        flush(position, mode);
      mTags[getLine(position)] = getTag(address);
      mValid[getLine(position)] = true;
      break;
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
        sendRequest(header);

        if (ENERGY_TRACE)
          Instrumentation::MemoryBank::initiateBurstRead(id.globalMemoryNumber());

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
        assert(false && "Memory bank can't handle off-chip requests");
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
        assert(false && "Memory bank can't handle off-chip requests");
        break;
    }
  }

  assert(isPayload(request));
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

uint32_t MemoryBank::readWord(SRAMAddress position) const {
  if (WARN_UNALIGNED && (position & 0x3) != 0)
    std::cerr << "Warning: Attempting to access address 0x" << std::hex << position
        << std::dec << " with alignment 4." << std::endl;

  return mData[position/4];
}

uint32_t MemoryBank::readHalfword(SRAMAddress position) const {
  if (WARN_UNALIGNED && (position & 0x1) != 0)
    std::cerr << "Warning: Attempting to access address 0x" << std::hex << position
        << std::dec << " with alignment 2." << std::endl;

  uint32_t fullWord = readWord(position & ~0x3);
  return ((position & 0x3) == 0) ? (fullWord & 0xFFFFUL) : (fullWord >> 16); // Little endian
}

uint32_t MemoryBank::readByte(SRAMAddress position) const {
  uint32_t fullWord = readWord(position & ~0x3);
  uint32_t selector = position & 0x3UL;

  switch (selector) { // Little endian
    case 0: return  fullWord        & 0xFFUL; break;
    case 1: return (fullWord >> 8)  & 0xFFUL; break;
    case 2: return (fullWord >> 16) & 0xFFUL; break;
    case 3: return (fullWord >> 24) & 0xFFUL; break;
    default: assert(false); return 0; break;
  }
}

void MemoryBank::writeWord(SRAMAddress position, uint32_t data) {
  MemoryAddr address = mTags[getLine(position)] + getOffset(position);
  if (WARN_UNALIGNED && (position & 0x3) != 0)
    LOKI_WARN << " attempting to access address " << LOKI_HEX(address)
        << " with alignment 4." << std::endl;

  mData[position/4] = data;
  mDirty[getLine(position)] = true;

  mReservations.clearReservation(address);
}

void MemoryBank::writeHalfword(SRAMAddress position, uint32_t data) {
  if (WARN_UNALIGNED && (position & 0x1) != 0)
    std::cerr << "Warning: Attempting to access address 0x" << std::hex << position
        << std::dec << " with alignment 2." << std::endl;

  uint32_t oldData = readWord(position & ~0x3);
  uint32_t newData;

  if ((position & 0x3) == 0) // Little endian
    newData = (oldData & 0xFFFF0000UL) | (data & 0x0000FFFFUL);
  else
    newData = (oldData & 0x0000FFFFUL) | (data << 16);

  writeWord(position & ~0x3, newData);
}

void MemoryBank::writeByte(SRAMAddress position, uint32_t data) {
  uint32_t oldData = readWord(position & ~0x3);
  uint32_t selector = position & 0x3UL;
  uint32_t newData;

  switch (selector) { // Little endian
  case 0: newData = (oldData & 0xFFFFFF00UL) | (data & 0x000000FFUL);        break;
  case 1: newData = (oldData & 0xFFFF00FFUL) | ((data & 0x000000FFUL) << 8);   break;
  case 2: newData = (oldData & 0xFF00FFFFUL) | ((data & 0x000000FFUL) << 16);    break;
  case 3: newData = (oldData & 0x00FFFFFFUL) | ((data & 0x000000FFUL) << 24);    break;
  }

  writeWord(position & ~0x3, newData);
}

//-------------------------------------------------------------------------------------------------
// Internal functions
//-------------------------------------------------------------------------------------------------

void MemoryBank::processIdle() {
  assert(mState == STATE_IDLE);

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

    if (MEMORY_HIT_UNDER_MISS && mServingMiss) {
      // Don't reorder data being sent to the same channel.
      if (mMissingRequest->getDestination() == mActiveRequest->getDestination())
        return;

      // Don't start another request if it will miss.
      if (!mActiveRequest->inCache())
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
  assert(mState == STATE_REQUEST);
  assert(mActiveRequest != NULL);

  // Before we even consider serving a request, we must make sure that there
  // is space to buffer any potential results.

  if (!canSendRequest()) {
    LOKI_LOG << this->name() << " delayed request due to full output request queue" << endl;
    next_trigger(canSendRequestEvent());
  }
  else if (mActiveRequest->resultsToSend() && !canSendResponse(mActiveRequest->getMemoryLevel())) {
    LOKI_LOG << this->name() << " delayed request due to full output queue" << endl;
    next_trigger(canSendResponseEvent(mActiveRequest->getMemoryLevel()));
  }
  else if (!iClock.negedge())
    next_trigger(iClock.negedge_event());
  else if (!mActiveRequest->preconditionsMet())
    mActiveRequest->prepare();
  else if (!mActiveRequest->complete())
    mActiveRequest->execute();
  else {
    mState = STATE_IDLE;
    mReadFromMissBuffer = false;
    delete mActiveRequest;
    mActiveRequest = NULL;
  }
}

void MemoryBank::processAllocate() {
  assert(mState == STATE_ALLOCATE);
  assert(mActiveRequest != NULL);

  // Use inCache to check whether the line has already been allocated. The data
  // won't have arrived yet.
  if (!mActiveRequest->inCache()) {
    // The request has already called allocate() above, so data for the new line
    // is on its way. Now we must prepare the line for the data's arrival.
    mActiveRequest->validateLine();

    // Put the request to one side until its data comes back.
    mMissingRequest = mActiveRequest;
    mCacheMissEvent.notify(sc_core::SC_ZERO_TIME);
    mServingMiss = true;

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
  assert(mState == STATE_FLUSH);
  assert(mActiveRequest != NULL);
  // It is assumed that the header flit has already been sent. All that is left
  // is to send the cache line.

  if (!iClock.negedge())
    next_trigger(iClock.negedge_event());
  if (canSendRequest()) {
    SRAMAddress position = getTag(mActiveRequest->getSRAMAddress()) + mCacheLineCursor;
    uint32_t data = readWord(position);

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
  assert(mState == STATE_REFILL);
  assert(mMissingRequest != NULL);

  if (!iClock.negedge())
    next_trigger(iClock.negedge_event());
  if (responseAvailable()) {
    SRAMAddress position = getTag(mMissingRequest->getSRAMAddress()) + mCacheLineCursor;
    uint32_t data = getResponse();
    writeWord(position, data);

    mCacheLineCursor += BYTES_PER_WORD;

    // Refill has finished if the cursor has covered a whole cache line.
    if (mCacheLineCursor >= CACHE_LINE_BYTES) {
      mState = STATE_REQUEST;
      mActiveRequest = mMissingRequest;
      mMissingRequest = NULL;
      mServingMiss = false;
      mReadFromMissBuffer = true;
      mRefillCompleteEvent.notify(sc_core::SC_ZERO_TIME);

      // Storing the requested words will have dirtied the cache line, but it
      // is actually clean.
      mDirty[getLine(position)] = false;

      LOKI_LOG << this->name() << " resuming " << memoryOpName(mActiveRequest->getMetadata().opcode) << " request" << endl;
    }
    else
      next_trigger(responseAvailableEvent());
  }
  else
    next_trigger(responseAvailableEvent());
}

bool MemoryBank::requestAvailable() const {
  return (!mInputQueue.empty() && !isPayload(mInputQueue.peek()))
      || (requestSig.valid() && !isPayload(requestSig.read()));
}

const sc_event_or_list& MemoryBank::requestAvailableEvent() const {
  return mInputQueue.writeEvent() | requestSig.default_event();
}

MemoryOperation* MemoryBank::peekRequest() {
  assert(requestAvailable());

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
    assert(requestSig.valid() && !isPayload(requestSig.read()));
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
  assert(requestAvailable());

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
      assert(false && "Memory bank can't handle off-chip requests");
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
  assert(canSendRequest());

  LOKI_LOG << this->name() << " buffering request " <<
      memoryOpName(request.getMemoryMetadata().opcode) << " " << request << endl;
  mOutputReqQueue.write(request);
}

bool MemoryBank::responseAvailable() const {
  return iResponse.valid() && (iResponseTarget.read() == id.position-CORES_PER_TILE);
}

const sc_event& MemoryBank::responseAvailableEvent() const {
  return iResponse.default_event();
}

uint32_t MemoryBank::getResponse() {
  assert(responseAvailable());
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
      assert(false && "Memory bank can't handle off-chip requests");
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
      assert(false && "Memory bank can't handle off-chip requests");
      return mOutputQueue.readEvent();
  }
}

void MemoryBank::sendResponse(NetworkResponse response, MemoryLevel level) {
  assert(canSendResponse(level));

  switch (level) {
    case MEMORY_L1:
      mOutputQueue.write(response);
      break;
    case MEMORY_L2:
      oResponse.write(response);
      break;
    default:
      assert(false && "Memory bank can't handle off-chip requests");
      break;
  }
}

bool MemoryBank::isPayload(NetworkRequest request) const {
  MemoryOpcode opcode = request.getMemoryMetadata().opcode;
  return (opcode == PAYLOAD) || (opcode == PAYLOAD_EOP);
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
        assert(false && "Memory bank can't handle off-chip requests");
        dataAvailable = false;
        break;
    }

    if (!dataAvailable) {
      next_trigger(requestAvailableEvent());
      return;
    }

    assert(isPayload(payload));
    assert(!mMissBuffer.full());
    mMissBuffer.write(payload);

    switch (mMissingRequest->getMemoryLevel()) {
      case MEMORY_L1:
        mInputQueue.read();
        break;
      case MEMORY_L2:
        requestSig.ack();
        break;
      default:
        assert(false && "Memory bank can't handle off-chip requests");
        break;
    }

    if (payload.getMetadata().endOfPacket)
      mCopyToMissBuffer = false;
    else
      next_trigger(iClock.negedge_event());
  }
}

void MemoryBank::preWriteCheck(MemoryAddr address) const {
  if (mBackgroundMemory->readOnly(address)) {
    if (WARN_READ_ONLY)
      LOKI_WARN << this->name() << " attempting to modify read-only address " << LOKI_HEX(address) << endl;
    else
      throw ReadOnlyException(address);
  }
}


void MemoryBank::processValidInput() {
  LOKI_LOG << this->name() << " received " << iData.read() << endl;
  assert(iData.valid());
  assert(!mInputQueue.full());

  mInputQueue.write(iData.read());
  iData.ack();
}

void MemoryBank::handleDataOutput() {
  // If we have new data to send:
  if (!mOutputWordPending) {
    if (mOutputQueue.empty()) {
      next_trigger(mOutputQueue.writeEvent());
    }
    else {
      mActiveOutputWord = mOutputQueue.read();
      mOutputWordPending = true;

      localNetwork->makeRequest(id, mActiveOutputWord.channelID(), true);
      next_trigger(iClock.negedge_event());
    }
  }
  // Check to see whether we are allowed to send data yet.
  else if (!localNetwork->requestGranted(id, mActiveOutputWord.channelID())) {
    // If not, wait another cycle.
    next_trigger(iClock.negedge_event());
  }
  // If it is time to send data:
  else {
    LOKI_LOG << this->name() << " sent " << mActiveOutputWord << endl;
    if (ENERGY_TRACE)
      Instrumentation::Network::traffic(id, mActiveOutputWord.channelID().component);

    oData.write(mActiveOutputWord);

    // Remove the request for network resources.
    if (mActiveOutputWord.getMetadata().endOfPacket)
      localNetwork->makeRequest(id, mActiveOutputWord.channelID(), false);

    mOutputWordPending = false;

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

    assert(!oRequest.valid());
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
  }

  updateIdle();
}

void MemoryBank::updateIdle() {
  bool wasIdle = currentlyIdle;
  currentlyIdle = mState == STATE_IDLE &&
                  mInputQueue.empty() && mOutputQueue.empty() &&
                  mOutputReqQueue.empty() &&
                  !mOutputWordPending;

  if (wasIdle != currentlyIdle)
    Instrumentation::idle(id, currentlyIdle);
}

void MemoryBank::updateReady() {
  bool ready = !mInputQueue.full();
  if (ready != oReadyForData.read())
    oReadyForData.write(ready);
}

//-------------------------------------------------------------------------------------------------
// Constructors and destructors
//-------------------------------------------------------------------------------------------------

MemoryBank::MemoryBank(sc_module_name name, const ComponentID& ID, uint bankNumber) :
	Component(name, ID),
  mInputQueue(string(this->name()) + string(".mInputQueue"), IN_CHANNEL_BUFFER_SIZE),
  mOutputQueue("mOutputQueue", OUT_CHANNEL_BUFFER_SIZE, INTERNAL_LATENCY),
  mOutputReqQueue("mOutputReqQueue", 10 /*read addr + write addr + cache line*/, 0),
  mData(MEMORY_BANK_SIZE),
  mTags(CACHE_LINES_PER_BANK),
  mValid(CACHE_LINES_PER_BANK),
  mDirty(CACHE_LINES_PER_BANK),
  mReservations(1),
	mMissBuffer(CACHE_LINE_WORDS, "mMissBuffer"),
	mCacheMissEvent(sc_core::sc_gen_unique_name("mCacheMissEvent")),
	mRefillCompleteEvent(sc_core::sc_gen_unique_name("mRefillCompleteEvent")),
	mL2RequestFilter("request_filter", ID, this)
{
	//-- Data queue state -------------------------------------------------------------------------

	mOutputWordPending = false;
	mActiveOutputWord = NetworkData();

	//-- Mode independent state -------------------------------------------------------------------
	mActiveRequest = NULL;
	mMissingRequest = NULL;

	mState = STATE_IDLE;
	mPreviousState = STATE_IDLE;

  mValid.assign(CACHE_LINES_PER_BANK, false);

	mCacheLineCursor = 0;

	mServingMiss = false;
	mCopyToMissBuffer = false;
	mReadFromMissBuffer = false;

	//-- Debug utilities --------------------------------------------------------------------------

	mBackgroundMemory = 0;
	localNetwork = 0;

	//-- Port initialization ----------------------------------------------------------------------

	oReadyForData.initialize(false);

	mL2RequestFilter.iClock(iClock);
	mL2RequestFilter.iRequest(iRequest);
	mL2RequestFilter.iRequestTarget(iRequestTarget);
	mL2RequestFilter.oRequest(requestSig);
	mL2RequestFilter.iRequestClaimed(iRequestClaimed);
	mL2RequestFilter.oClaimRequest(oClaimRequest);

	//-- Register module with SystemC simulation kernel -------------------------------------------

	currentlyIdle = true;
	Instrumentation::idle(id, true);

	SC_METHOD(mainLoop);
	sensitive << iClock.neg();
	dont_initialize();

	SC_METHOD(updateReady);
	sensitive << mInputQueue.readEvent() << mInputQueue.writeEvent();
	// do initialise

	SC_METHOD(processValidInput);
	sensitive << iData;
	dont_initialize();

	SC_METHOD(handleDataOutput);

  SC_METHOD(handleRequestOutput);

  SC_METHOD(copyToMissBuffer);
  sensitive << mCacheMissEvent;
  dont_initialize();

	end_module(); // Needed because we're using a different Component constructor
}

MemoryBank::~MemoryBank() {
}

void MemoryBank::setLocalNetwork(local_net_t* network) {
  localNetwork = network;
}

void MemoryBank::setBackgroundMemory(SimplifiedOnChipScratchpad* memory) {
  mBackgroundMemory = memory;
}

void MemoryBank::storeData(vector<Word>& data, MemoryAddr location) {
	assert(false);
}

void MemoryBank::print(MemoryAddr start, MemoryAddr end) {
	assert(mBackgroundMemory != NULL);

	if (start > end) {
		MemoryAddr temp = start;
		start = end;
		end = temp;
	}

	size_t address = (start / 4) * 4;
	size_t limit = (end / 4) * 4 + 4;

  while (address < limit) {
    uint32_t data = readWordDebug(address).toUInt();
    cout << "0x" << setprecision(8) << setfill('0') << hex << address << ":  " << "0x" << setprecision(8) << setfill('0') << hex << data << dec << endl;
    address += 4;
  }
}

Word MemoryBank::readWordDebug(MemoryAddr addr) {
	assert(mBackgroundMemory != NULL);
	assert(addr % 4 == 0);

	SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    return readWord(position);
  else
    return mBackgroundMemory->readWord(addr);
}

Word MemoryBank::readByteDebug(MemoryAddr addr) {
	assert(mBackgroundMemory != NULL);

  SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    return readByte(position);
  else
    return mBackgroundMemory->readByte(addr);
}

void MemoryBank::writeWordDebug(MemoryAddr addr, Word data) {
	assert(mBackgroundMemory != NULL);
	assert(addr % 4 == 0);

  SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    writeWord(position, data.toUInt());
  else
    mBackgroundMemory->writeWord(addr, data.toUInt());
}

void MemoryBank::writeByteDebug(MemoryAddr addr, Word data) {
	assert(mBackgroundMemory != NULL);

  SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    writeByte(position, data.toUInt());
  else
    mBackgroundMemory->writeByte(addr, data.toUInt());
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

void MemoryBank::printOperation(MemoryOpcode operation,
                                MemoryAddr                     address,
                                uint32_t                       data) const {
  LOKI_LOG << this->name() << ": " << memoryOpName(operation) <<
      ", address = " << LOKI_HEX(address) << ", data = " << LOKI_HEX(data);

  if (operation == IPK_READ) {
    Instruction inst(data);
    LOKI_LOG << " (" << inst << ")";
  }

  LOKI_LOG << endl;
}
