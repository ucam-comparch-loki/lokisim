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

using namespace std;

#include <iostream>
#include <iomanip>
#include "MemoryBank.h"
#include "../../OffChip/MainMemory.h"
#include "../../Datatype/MemoryOperations/MemoryOperationDecode.h"
#include "../ComputeTile.h"
#include "../../Chip.h"
#include "../../Utility/Arguments.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/Latency.h"
#include "../../Utility/Instrumentation/MemoryBank.h"
#include "../../Utility/Instrumentation/Network.h"
#include "../../Utility/Parameters.h"
#include "../../Utility/Warnings.h"
#include "../../Exceptions/ReadOnlyException.h"
#include "../../Exceptions/UnsupportedFeatureException.h"

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
    Instrumentation::MemoryBank::checkTags(id, address);

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
  return tags[getLine(position)] + getOffset(position);
}

// Return whether data from `address` can be found at `position` in the SRAM.
bool MemoryBank::contains(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) const {
  switch (mode) {
    case MEMORY_CACHE: {
      uint cacheLine = getLine(position);
      return (tags[cacheLine] == getTag(address)) && valid[cacheLine];
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
        Instrumentation::MemoryBank::replaceCacheLine(id, valid[getLine(position)], dirty[getLine(position)]);

        // Send a request for the missing cache line.
        NetworkRequest readRequest(getTag(address), id, FETCH_LINE, true);
        MemoryMetadata metadata = readRequest.getMemoryMetadata();
        metadata.skipL2 = activeRequest->getMetadata().skipL2;
        readRequest.setMetadata(metadata.flatten());
        sendRequest(readRequest);

        // Stop serving the request until allocation is complete.
        state = STATE_ALLOCATE;
        next_trigger(sc_core::SC_ZERO_TIME); // Continue immediately.
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
      if (tags[getLine(position)] != getTag(address))
        flush(position, mode);
      tags[getLine(position)] = getTag(address);
      valid[getLine(position)] = true;

      // Hack. It might not always be the active request doing this?
      l2Skip[getLine(position)] = activeRequest->getMetadata().skipL2;

      // Some extra bookkeeping to help with debugging.
      bool inMainMemory = chip()->backedByMainMemory(id.tile, address);
      if (inMainMemory) {
        MemoryAddr globalAddress = chip()->getAddressTranslation(id.tile, address);
        mainMemory->claimCacheLine(id, globalAddress);
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
      valid[getLine(position)] = false;
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
      if (valid[getLine(position)] && dirty[getLine(position)]) {
        // Send a header flit telling where this line should be stored.
        MemoryAddr address = tags[getLine(position)];
        NetworkRequest header(address, id, STORE_LINE, false);
        MemoryMetadata metadata = header.getMemoryMetadata();
        metadata.skipL2 = l2Skip[getLine(position)];
        header.setMetadata(metadata.flatten());
        sendRequest(header);

        pendingFlushes.push_back(address);

        if (ENERGY_TRACE)
          Instrumentation::MemoryBank::startOperation(id, FLUSH_LINE,
              tags[getLine(position)], false, ChannelID(id,0));

        // The flush state handles sending the line itself.
        previousState = state;
        state = STATE_FLUSH;
        cacheLineCursor = 0;
      }
      else
        state = STATE_REQUEST;

      dirty[getLine(position)] = false;
      break;
    case MEMORY_SCRATCHPAD:
      break;
  }
}

// Return whether a payload flit is available. `level` tells whether this bank
// is being treated as an L1 or L2 cache.
bool MemoryBank::payloadAvailable(MemoryLevel level) const {
  if (readingFromMissBuffer) {
    return !missBuffer.empty();
  }
  else {
    switch (level) {
      case MEMORY_L1:
        return !inputQueue.empty() && isPayload(inputQueue.peek());
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

  if (readingFromMissBuffer) {
    request = missBuffer.read();
  }
  else {
    switch (level) {
      case MEMORY_L1:
        request = inputQueue.read();
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
void MemoryBank::makeReservation(ComponentID requester, MemoryAddr address, MemoryAccessMode mode) {
  reservations.makeReservation(requester, address, getPosition(address, mode));
}

// Return whether a load-linked reservation is still valid.
bool MemoryBank::checkReservation(ComponentID requester, MemoryAddr address, MemoryAccessMode mode) const {
  return reservations.checkReservation(requester, address);
}

void MemoryBank::writeWord(SRAMAddress position, uint32_t value, MemoryAccessMode mode) {
  checkAlignment(position, BYTES_PER_WORD);

  data[position/BYTES_PER_WORD] = value;
  if (mode == MEMORY_CACHE)
    dirty[getLine(position)] = true;

  reservations.clearReservation(position);
}

const sc_event& MemoryBank::requestSentEvent() const {
  return outputReqQueue.readEvent();
}

bool MemoryBank::flushing(MemoryAddr address) const {
  MemoryAddr cacheLine = getTag(address);

  for (uint i=0; i<pendingFlushes.size(); i++) {
    if (pendingFlushes[i] == cacheLine) {
      return true;
    }
  }

  return false;
}

void MemoryBank::processIdle() {
  loki_assert_with_message(state == STATE_IDLE, "State = %d", state);

  // Refills have priority because other requests may depend on them.
  if (responseAvailable()) {
    state = STATE_REFILL;
    cacheLineCursor = 0;
    next_trigger(sc_core::SC_ZERO_TIME);
  }
  // Check for any other requests.
  else if (requestAvailable()) {
    // If the hit-under-miss option is enabled and we are currently serving a
    // miss, peek the next request in the queue to see if it is a hit.

    activeRequest = peekRequest();

    if (MEMORY_HIT_UNDER_MISS && (missingRequest != NULL)) {
      // Don't reorder data being sent to the same channel.
      if (missingRequest->getDestination() == activeRequest->getDestination())
        return;

      // Don't start a request for the same location as the missing request.
      if (getTag(missingRequest->getAddress()) == getTag(activeRequest->getAddress()))
        return;

      // Don't start another request if it will miss.
      if (!activeRequest->inCache())
        return;

      // Don't start a request which skips this cache, but will return data
      // through it.
      if (activeRequest->needsForwarding() && activeRequest->resultsToSend())
        return;

      LOKI_LOG << this->name() << " starting hit-under-miss" << endl;
    }

    consumeRequest(activeRequest->getMemoryLevel());
    state = STATE_REQUEST;
    next_trigger(sc_core::SC_ZERO_TIME);

    LOKI_LOG << this->name() << " starting " << memoryOpName(activeRequest->getMetadata().opcode)
        << " request from component " << activeRequest->getDestination().component << endl;
  }
  // Nothing to do - wait for input to arrive.
  else {
    next_trigger(responseAvailableEvent() | requestAvailableEvent());
  }
}

void MemoryBank::processRequest() {
  loki_assert_with_message(state == STATE_REQUEST, "State = %d", state);
  loki_assert(activeRequest != NULL);

  // Before we even consider serving a request, we must make sure that there
  // is space to buffer any potential results.

  if (!canSendRequest()) {
    LOKI_LOG << this->name() << " delayed request due to full output request queue" << endl;
    next_trigger(canSendRequestEvent());
  }
  else if (activeRequest->needsForwarding()) {
    forwardRequest(activeRequest->getOriginal());
    if (activeRequest->awaitingPayload())
      state = STATE_FORWARD;
    else {
      loki_assert(missingRequest == NULL);
      missingRequest = activeRequest;
      finishedRequestForNow();
    }
  }
  else if (!activeRequest->preconditionsMet()) {
    activeRequest->prepare();
  }
  else if (activeRequest->resultsToSend() && !canSendResponse(activeRequest->getDestination(), activeRequest->getMemoryLevel())) {
    LOKI_LOG << this->name() << " delayed request due to full output queue" << endl;
    next_trigger(canSendResponseEvent(activeRequest->getDestination(), activeRequest->getMemoryLevel()));
  }
  else if (!iClock.negedge()) {
    next_trigger(iClock.negedge_event());
  }
  else if (!activeRequest->complete()) {
    activeRequest->execute();
  }

  // If the operation has finished, and we haven't moved to some other state
  // for further processing, end the request and prepare for a new one.
  if (activeRequest != NULL && activeRequest->complete() && state == STATE_REQUEST) {
    finishedRequest();
    readingFromMissBuffer = false;
  }
}

void MemoryBank::processAllocate() {
  loki_assert_with_message(state == STATE_ALLOCATE, "State = %d", state);
  loki_assert(activeRequest != NULL);

  if (!canSendRequest()) {
    LOKI_LOG << this->name() << " delayed allocation due to full output request queue" << endl;
    next_trigger(canSendRequestEvent());
  }
  // Use inCache to check whether the line has already been allocated. The data
  // won't have arrived yet.
  else if (!activeRequest->inCache()) {
    // The request has already called allocate() above, so data for the new line
    // is on its way. Now we must prepare the line for the data's arrival.
    activeRequest->validateLine();

    // Put the request to one side until its data comes back.
    missingRequest = activeRequest;
    missingRequest->notifyCacheMiss();
    cacheMissEvent.notify(sc_core::SC_ZERO_TIME);

    if (missingRequest->awaitingPayload())
      copyingToMissBuffer = true;

    if (state != STATE_FLUSH) {
      state = STATE_IDLE;
      activeRequest.reset();
    }
  }
  else {
    state = STATE_IDLE;
    activeRequest.reset();
  }
}

void MemoryBank::processFlush() {
  loki_assert_with_message(state == STATE_FLUSH, "State = %d", state);
  loki_assert(activeRequest != NULL);
  // It is assumed that the header flit has already been sent. All that is left
  // is to send the cache line.

  if (!iClock.negedge())
    next_trigger(iClock.negedge_event());
  else if (canSendRequest()) {
    SRAMAddress position = getTag(activeRequest->getSRAMAddress()) + cacheLineCursor;
    uint32_t data = readWord(position, activeRequest->getAccessMode());

    Instrumentation::MemoryBank::continueOperation(id, FLUSH_LINE,
        tags[getLine(position)] + cacheLineCursor, false, ChannelID(id,0));

    cacheLineCursor += BYTES_PER_WORD;

    // Flush has finished if the cursor has covered a whole cache line.
    bool endOfPacket = cacheLineCursor >= CACHE_LINE_BYTES;
    if (endOfPacket)
      state = previousState;

    NetworkRequest flit(data, id, PAYLOAD, endOfPacket);
    sendRequest(flit);
  }
  else
    next_trigger(canSendRequestEvent());
}

void MemoryBank::processRefill() {
  loki_assert_with_message(state == STATE_REFILL, "State = %d", state);
  loki_assert(missingRequest != NULL);

  if (!iClock.negedge())
    next_trigger(iClock.negedge_event());
  else if (responseAvailable()) {

    // Don't store data locally if the request bypasses this cache.
    if (missingRequest->needsForwarding()) {
      if (canSendResponse(missingRequest->getDestination(), missingRequest->getMemoryLevel())) {
        uint32_t data = getResponse();
        missingRequest->sendResult(data);

        if (missingRequest->resultsToSend()) {
          next_trigger(responseAvailableEvent());
        }
        else {
          state = STATE_IDLE;
          missingRequest.reset();
        }
      }
      else
        next_trigger(canSendResponseEvent(missingRequest->getDestination(), missingRequest->getMemoryLevel()));
    }
    else {
      uint32_t data = getResponse();
      SRAMAddress position = getTag(missingRequest->getSRAMAddress()) + cacheLineCursor;
      writeWord(position, data, missingRequest->getAccessMode());

      cacheLineCursor += BYTES_PER_WORD;

      // Refill has finished if the cursor has covered a whole cache line.
      if (cacheLineCursor >= CACHE_LINE_BYTES) {
        state = STATE_REQUEST;
        activeRequest = missingRequest;
        missingRequest.reset();
        readingFromMissBuffer = true;

        // Storing the requested words will have dirtied the cache line, but it
        // is actually clean.
        dirty[getLine(position)] = false;

        LOKI_LOG << this->name() << " resuming " << memoryOpName(activeRequest->getMetadata().opcode) << " request" << endl;
      }
      else
        next_trigger(responseAvailableEvent());
    }
  }
  else
    next_trigger(responseAvailableEvent());
}

void MemoryBank::processForward() {
  loki_assert_with_message(state == STATE_FORWARD, "State = %d", state);
  loki_assert(activeRequest != NULL);

  // Before we even consider serving a request, we must make sure that there
  // is space to buffer any potential results.

  if (!canSendRequest()) {
    LOKI_LOG << this->name() << " delayed request due to full output request queue" << endl;
    next_trigger(canSendRequestEvent());
  }
  else if (!payloadAvailable(activeRequest->getMemoryLevel())) {
    next_trigger(requestAvailableEvent());
  }
  else {
    NetworkRequest payload;
    switch (activeRequest->getMemoryLevel()) {
      case MEMORY_L1:
        payload = inputQueue.read();
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
      if (activeRequest->resultsToSend()) {
        loki_assert(missingRequest == NULL);
        missingRequest = activeRequest;
        finishedRequestForNow();
      }
      else
        finishedRequest();
    }
  }
}

void MemoryBank::finishedRequest() {
  finishedRequestForNow();
}

void MemoryBank::finishedRequestForNow() {
  state = STATE_IDLE;
  activeRequest.reset();

  // Decode the next request immediately so it is ready to start next cycle.
  next_trigger(sc_core::SC_ZERO_TIME);
}

bool MemoryBank::requestAvailable() const {
  return (!inputQueue.empty() && !isPayload(inputQueue.peek()))
      || (requestSig.valid() && !isPayload(requestSig.read()));
}

const sc_event_or_list& MemoryBank::requestAvailableEvent() const {
  return inputQueue.writeEvent() | requestSig.default_event();
}

std::shared_ptr<MemoryOperation> MemoryBank::peekRequest() {
  loki_assert(requestAvailable());

  NetworkRequest request;
  MemoryLevel level;
  ChannelID destination;

  if (!inputQueue.empty() && !isPayload(inputQueue.peek())) {
    request = inputQueue.peek();
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

  return std::shared_ptr<MemoryOperation>(decodeMemoryRequest(request, *this, level, destination));
}

void MemoryBank::consumeRequest(MemoryLevel level) {
  loki_assert(requestAvailable());

  switch (level) {
    case MEMORY_L1: {
      NetworkRequest request = inputQueue.read();
      LOKI_LOG << this->name() << " dequeued " << request << endl;
      Instrumentation::Latency::memoryStartedRequest(id, request);
      break;
    }
    case MEMORY_L2:
      requestSig.ack();
      Instrumentation::Latency::memoryStartedRequest(id, requestSig.read());
      break;
    default:
      loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
      break;
  }
}

bool MemoryBank::canSendRequest() const {
  return !outputReqQueue.full();
}

const sc_event& MemoryBank::canSendRequestEvent() const {
  return outputReqQueue.readEvent();
}

void MemoryBank::sendRequest(NetworkRequest request) {
  if (!canSendRequest()) assert(false);
  loki_assert(canSendRequest());

  LOKI_LOG << this->name() << " buffering request " <<
      memoryOpName(request.getMemoryMetadata().opcode) << " " << request << endl;
  outputReqQueue.write(request);
}

void MemoryBank::forwardRequest(NetworkRequest request) {
  // Need to update the return address to point to this bank rather than the
  // original requester.
  LOKI_LOG << this->name() << " bypassed by request " << request << endl;
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

bool MemoryBank::canSendResponse(ChannelID destination, MemoryLevel level) const {
  switch (level) {
    case MEMORY_L1:
      if (destination.channel < CORE_INSTRUCTION_CHANNELS)
        return !outputInstQueue.full();
      else
        return !outputDataQueue.full();
    case MEMORY_L2:
      return !oResponse.valid();
    default:
      loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
      return false;
  }
}

const sc_event& MemoryBank::canSendResponseEvent(ChannelID destination, MemoryLevel level) const {
  switch (level) {
    case MEMORY_L1:
      if (destination.channel < CORE_INSTRUCTION_CHANNELS)
        return outputInstQueue.readEvent();
      else
        return outputDataQueue.readEvent();
    case MEMORY_L2:
      return oResponse.ack_event();
    default:
      loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
      return outputDataQueue.readEvent();
  }
}

void MemoryBank::sendResponse(NetworkResponse response, MemoryLevel level) {
  loki_assert(canSendResponse(response.channelID(), level));

  // Instrumentation for buffering the result is in MemoryOperation because
  // it uses extra information about the request.

  switch (level) {
    case MEMORY_L1:
      if (response.channelID().channel < CORE_INSTRUCTION_CHANNELS) {
        LOKI_LOG << this->name() << " buffering instruction " << response << endl;
        outputInstQueue.write(response);
      }
      else {
        LOKI_LOG << this->name() << " buffering data " << response << endl;
        outputDataQueue.write(response);
      }
      break;
    case MEMORY_L2:
      oResponse.write(response);
      Instrumentation::Latency::memorySentResult(id, response, false);
      break;
    default:
      loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
      break;
  }
}

void MemoryBank::copyToMissBuffer() {
  if (copyingToMissBuffer) {
    // Check which input the missing request was reading from, and while
    // payloads arrive there, copy them into the miss buffer.
    NetworkRequest payload;
    bool dataAvailable;

    switch (missingRequest->getMemoryLevel()) {
      case MEMORY_L1:
        dataAvailable = !inputQueue.empty();
        if (dataAvailable)
          payload = inputQueue.peek();
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
    loki_assert(!missBuffer.full());
    missBuffer.write(payload);

    switch (missingRequest->getMemoryLevel()) {
      case MEMORY_L1:
        inputQueue.read();
        LOKI_LOG << this->name() << " dequeued " << payload << endl;
        break;
      case MEMORY_L2:
        requestSig.ack();
        break;
      default:
        loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
        break;
    }

    if (payload.getMetadata().endOfPacket)
      copyingToMissBuffer = false;
    else
      next_trigger(iClock.negedge_event());
  }
}

void MemoryBank::preWriteCheck(const MemoryOperation& operation) const {
  MemoryAddr globalAddress = chip()->getAddressTranslation(id.tile, operation.getAddress());
  bool scratchpad = operation.getAccessMode() == MEMORY_SCRATCHPAD;
  bool inMainMemory = !scratchpad && chip()->backedByMainMemory(id.tile, operation.getAddress());
  if (inMainMemory && mainMemory->readOnly(globalAddress) && WARN_READ_ONLY) {
    LOKI_WARN << this->name() << " attempting to modify read-only address" << endl;
    LOKI_WARN << "  " << operation.toString() << endl;
  }
}

void MemoryBank::processValidInput() {
  loki_assert(iData.valid());
  LOKI_LOG << this->name() << " received " << iData.read() << endl;
  loki_assert(!inputQueue.full());
  inputQueue.write(iData.read());
  iData.ack();
  Instrumentation::Network::recordBandwidth(iData.name());
  Instrumentation::Latency::memoryReceivedRequest(id, iData.read());
}

void MemoryBank::handleDataOutput() {
  if (outputDataQueue.empty()) {
    next_trigger(outputDataQueue.writeEvent());
  }
  else if (!parent()->requestGranted(id, outputDataQueue.peek().channelID())) {
    parent()->makeRequest(id, outputDataQueue.peek().channelID(), true);
    next_trigger(iClock.negedge_event());
  }
  else {
    NetworkResponse flit = outputDataQueue.read();
    LOKI_LOG << this->name() << " sent data " << flit << endl;
    if (ENERGY_TRACE)
      Instrumentation::Network::traffic(id, flit.channelID().component);
    Instrumentation::Network::recordBandwidth(oData.name());
    Instrumentation::Latency::memorySentResult(id, flit, true);

    oData.write(flit);

    // Remove the request for network resources.
    if (flit.getMetadata().endOfPacket)
      parent()->makeRequest(id, flit.channelID(), false);

    next_trigger(oData.ack_event());
  }
}

void MemoryBank::handleInstructionOutput() {
  if (outputInstQueue.empty()) {
    next_trigger(outputInstQueue.writeEvent());
  }
  else if (!parent()->requestGranted(id, outputInstQueue.peek().channelID())) {
    parent()->makeRequest(id, outputInstQueue.peek().channelID(), true);
    next_trigger(iClock.negedge_event());
  }
  else {
    NetworkResponse flit = outputInstQueue.read();
    LOKI_LOG << this->name() << " sent instruction " << flit << endl;
    if (ENERGY_TRACE)
      Instrumentation::Network::traffic(id, flit.channelID().component);
    Instrumentation::Network::recordBandwidth(oInstruction.name());
    Instrumentation::Latency::memorySentResult(id, flit, true);

    oInstruction.write(flit);

    // Remove the request for network resources.
    if (flit.getMetadata().endOfPacket)
      parent()->makeRequest(id, flit.channelID(), false);

    next_trigger(oInstruction.ack_event());
  }
}

// Method which sends data from the mOutputReqQueue whenever possible.
void MemoryBank::handleRequestOutput() {
  if (!iClock.posedge())
    next_trigger(iClock.posedge_event());
  else if (outputReqQueue.empty())
    next_trigger(outputReqQueue.writeEvent());
  else {
    NetworkRequest request = outputReqQueue.read();
    LOKI_LOG << this->name() << " sent request " <<
        memoryOpName(request.getMemoryMetadata().opcode) << " " << LOKI_HEX(request.payload()) << endl;

    // Slightly hacky: if the output we are overwriting is the head of a flush
    // request, then we know that that request has now been selected to go out
    // on the network, and cannot be overtaken by any other requests. Therefore,
    // it is safe to remove it from our pendingFlushes queue.
    if (oRequest.read().getMemoryMetadata().opcode == STORE_LINE &&
        !pendingFlushes.empty() &&
        getTag(oRequest.read().payload().toUInt()) == pendingFlushes.front()) {
      pendingFlushes.erase(pendingFlushes.begin());
    }

    loki_assert(!oRequest.valid());
    oRequest.write(request);
    next_trigger(oRequest.ack_event());
  }
}

void MemoryBank::mainLoop() {
  switch (state) {
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
  currentlyIdle = state == STATE_IDLE &&
                  inputQueue.empty() && outputDataQueue.empty() &&
                  outputInstQueue.empty() && outputReqQueue.empty();

  if (wasIdle != currentlyIdle)
    Instrumentation::idle(id, currentlyIdle);
}

void MemoryBank::updateReady() {
  bool ready = !inputQueue.full();
  if (ready != oReadyForData.read())
    oReadyForData.write(ready);
}

ComputeTile* MemoryBank::parent() const {
  return static_cast<ComputeTile*>(this->get_parent_object());
}

Chip* MemoryBank::chip() const {
  return parent()->chip();
}

MemoryBank::MemoryBank(sc_module_name name, const ComponentID& ID) :
  MemoryBase(name, ID),
  iClock("iClock"),
  iData("iData"),
  oReadyForData("oReadyForData"),
  oData("oData"),
  oInstruction("oInstruction"),
  iRequest("iRequest"),
  iRequestTarget("iRequestTarget"),
  oRequest("oRequest"),
  iRequestClaimed("iRequestClaimed"),
  oClaimRequest("oClaimRequest"),
  iRequestDelayed("iRequestDelayed"),
  oDelayRequest("oDelayRequest"),
  iResponse("iResponse"),
  iResponseTarget("iResponseTarget"),
  oResponse("oResponse"),
  inputQueue(string(this->name()) + string(".mInputQueue"), MEMORY_BUFFER_SIZE),
  outputDataQueue("mOutputDataQueue", MEMORY_BUFFER_SIZE, INTERNAL_LATENCY),
  outputInstQueue("mOutputInstQueue", MEMORY_BUFFER_SIZE, INTERNAL_LATENCY),
  outputReqQueue("mOutputReqQueue", 10 /*read addr + write addr + cache line*/, 0),
  data(MEMORY_BANK_SIZE/BYTES_PER_WORD, 0),
  tags(CACHE_LINES_PER_BANK, 0),
  valid(CACHE_LINES_PER_BANK, false),
  dirty(CACHE_LINES_PER_BANK, false),
  l2Skip(CACHE_LINES_PER_BANK, false),
  reservations(1),
  missBuffer(CACHE_LINE_WORDS, "mMissBuffer"),
  cacheMissEvent(sc_core::sc_gen_unique_name("mCacheMissEvent")),
  l2RequestFilter("request_filter", ID, this)
{
  state = STATE_IDLE;
  previousState = STATE_IDLE;

  cacheLineCursor = 0;

  copyingToMissBuffer = false;
  readingFromMissBuffer = false;

  // Magic interfaces.
  mainMemory = NULL;

  // Connect to local components.
  oReadyForData.initialize(false);

  l2RequestFilter.iClock(iClock);
  l2RequestFilter.iRequest(iRequest);
  l2RequestFilter.iRequestTarget(iRequestTarget);
  l2RequestFilter.oRequest(requestSig);
  l2RequestFilter.iRequestClaimed(iRequestClaimed);
  l2RequestFilter.oClaimRequest(oClaimRequest);
  l2RequestFilter.iRequestDelayed(iRequestDelayed);
  l2RequestFilter.oDelayRequest(oDelayRequest);

  currentlyIdle = true;
  Instrumentation::idle(id, true);

  SC_METHOD(mainLoop);
  sensitive << iClock.neg();
  dont_initialize();

  SC_METHOD(updateReady);
  sensitive << inputQueue.readEvent() << inputQueue.writeEvent();
  // do initialise

  SC_METHOD(updateIdle);
  sensitive << inputQueue.readEvent() << inputQueue.writeEvent()
            << outputDataQueue.readEvent() << outputDataQueue.writeEvent()
            << outputInstQueue.readEvent() << outputInstQueue.writeEvent()
            << outputReqQueue.readEvent() << outputReqQueue.writeEvent()
            << iResponse << oResponse;
  // do initialise

  SC_METHOD(processValidInput);
  sensitive << iData;
  dont_initialize();

  SC_METHOD(handleDataOutput);
  SC_METHOD(handleInstructionOutput);
  SC_METHOD(handleRequestOutput);

  SC_METHOD(copyToMissBuffer);
  sensitive << cacheMissEvent;
  dont_initialize();
}

MemoryBank::~MemoryBank() {
}

void MemoryBank::setBackgroundMemory(MainMemory* memory) {
  assert(memory != NULL);
  mainMemory = memory;
}

void MemoryBank::print(MemoryAddr start, MemoryAddr end) {
  loki_assert(mainMemory != NULL);

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
  loki_assert(mainMemory != NULL);
  loki_assert_with_message(addr % 4 == 0, "Address = 0x%x", addr);

  SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    return readWord(position, MEMORY_CACHE);
  else
    return mainMemory->readWord(addr, MEMORY_SCRATCHPAD);
}

Word MemoryBank::readByteDebug(MemoryAddr addr) {
  loki_assert(mainMemory != NULL);

  SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    return readByte(position, MEMORY_CACHE);
  else
    return mainMemory->readByte(addr, MEMORY_SCRATCHPAD);
}

void MemoryBank::writeWordDebug(MemoryAddr addr, Word data) {
  loki_assert(mainMemory != NULL);
  loki_assert_with_message(addr % 4 == 0, "Address = 0x%x", addr);

  SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    writeWord(position, data.toUInt(), MEMORY_CACHE);
  else
    mainMemory->writeWord(addr, data.toUInt(), MEMORY_SCRATCHPAD);
}

void MemoryBank::writeByteDebug(MemoryAddr addr, Word data) {
  loki_assert(mainMemory != NULL);

  SRAMAddress position = getPosition(addr, MEMORY_CACHE);

  if (contains(addr, position, MEMORY_CACHE))
    writeByte(position, data.toUInt(), MEMORY_CACHE);
  else
    mainMemory->writeByte(addr, data.toUInt(), MEMORY_SCRATCHPAD);
}

const vector<uint32_t>& MemoryBank::dataArray() const {
  return data;
}

vector<uint32_t>& MemoryBank::dataArray() {
  return data;
}

void MemoryBank::reportStalls(ostream& os) {
  if (inputQueue.full()) {
    os << inputQueue.name() << " is full." << endl;
  }
  if (!outputDataQueue.empty()) {
    const NetworkResponse& outWord = outputDataQueue.peek();
    os << this->name() << " waiting to send " << outWord << endl;
  }
  if (!outputInstQueue.empty()) {
    const NetworkResponse& outWord = outputInstQueue.peek();
    os << this->name() << " waiting to send " << outWord << endl;
  }
  if (!outputReqQueue.empty()) {
    const NetworkResponse& outWord = outputReqQueue.peek();
    os << this->name() << " waiting to send " << outWord << endl;
  }
}
