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
#include "../../Datatype/MemoryOperations/MemoryOperationDecode.h"
#include "../ComputeTile.h"
#include "../../Chip.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/Latency.h"
#include "../../Utility/Instrumentation/L1Cache.h"
#include "../../Utility/Warnings.h"
#include "../../Exceptions/ReadOnlyException.h"
#include "../../Exceptions/UnsupportedFeatureException.h"

// The latency of accessing a memory bank, aside from the cost of accessing the
// SRAM. This typically includes the network to/from the bank.
#define EXTERNAL_LATENCY 1


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

size_t MemoryBank::numCacheLines() const {
  return metadata.size();
}

size_t MemoryBank::cacheLineSize() const {
  return 1 << log2CacheLineSize;
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
    Instrumentation::L1Cache::checkTags(id, address);

  static const uint indexBits = log2(numCacheLines());
  uint offset = getOffset(address);
  uint bank = (address >> log2CacheLineSize) & 0x7;
  uint index = (address >> (log2CacheLineSize + log2NumBanks)) & ((1 << indexBits) - 1);
  uint slot = index ^ (bank << (indexBits - log2NumBanks)); // Hash bank into the upper bits
  return (slot << log2CacheLineSize) | offset;
}

// Return the position in the memory address space of the data stored at the
// given position.
MemoryAddr MemoryBank::getAddress(SRAMAddress position) const {
  return metadata[getLine(position)].address + getOffset(position);
}

// Return whether data from `address` can be found at `position` in the SRAM.
bool MemoryBank::contains(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) const {
  switch (mode) {
    case MEMORY_CACHE: {
      TagData tag = metadata[getLine(position)];
      return (tag.address == getTag(address)) && tag.valid;
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
  // Only the hit request may call this method.
  loki_assert(hitRequest != NULL);

  switch (mode) {
    case MEMORY_CACHE:
      if (!contains(address, position, mode)) {
        LOKI_LOG << this->name() << " cache miss at address " << LOKI_HEX(address) << endl;
        TagData tag = metadata[getLine(position)];
        Instrumentation::L1Cache::replaceCacheLine(id, tag.valid, tag.dirty);

        // Send a request for the missing cache line.
        NetworkRequest readRequest(getTag(address), id, FETCH_LINE, true);
        MemoryMetadata metadata = readRequest.getMemoryMetadata();
        metadata.skipL2 = hitRequest->getMetadata().skipL2;
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
  // Only the hit request may call this method.
  loki_assert(hitRequest != NULL);

  switch (mode) {
    case MEMORY_CACHE: {
      TagData& tag = metadata[getLine(position)];

      if (tag.address != getTag(address))
        flush(position, mode);
      tag.address = getTag(address);
      tag.valid = true;

      // Hack. It might not always be the active request doing this?
      tag.l2Skip = hitRequest->getMetadata().skipL2;

      // Some extra bookkeeping to help with debugging.
      bool inMainMemory = chip().backedByMainMemory(id.tile, address);
      if (inMainMemory) {
        MemoryAddr globalAddress = chip().getAddressTranslation(id.tile, address);
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
  // Only the hit request may call this method.
  loki_assert(hitRequest != NULL);

  switch (mode) {
    case MEMORY_CACHE:
      metadata[getLine(position)].valid = false;
      break;
    case MEMORY_SCRATCHPAD:
      break;
  }
}

// Flush the cache line containing `position` down the memory hierarchy, if
// necessary. The line is not invalidated, but is no longer dirty.
void MemoryBank::flush(SRAMAddress position, MemoryAccessMode mode) {
  switch (mode) {
    case MEMORY_CACHE: {
      TagData& tag = metadata[getLine(position)];

      if (tag.valid && tag.dirty) {
        // Send a header flit telling where this line should be stored.
        MemoryAddr address = tag.address;
        NetworkRequest header(address, id, STORE_LINE, false);
        MemoryMetadata metadata = header.getMemoryMetadata();
        metadata.skipL2 = tag.l2Skip;
        header.setMetadata(metadata.flatten());
        sendRequest(header);

        pendingFlushes.push_back(address);

        if (ENERGY_TRACE)
          Instrumentation::L1Cache::startOperation(*this, FLUSH_LINE,
              tag.address, false, ChannelID(id,0));

        // The flush state handles sending the line itself.
        previousState = state;
        state = STATE_FLUSH;
        cacheLineCursor = 0;
      }
      else
        state = STATE_REQUEST;

      tag.dirty = false;
      break;
    }

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
        return inputQueue.dataAvailable() && isPayload(inputQueue.peek());
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
    metadata[getLine(position)].dirty = true;

  reservations.clearReservation(position);
}

const sc_event& MemoryBank::requestSentEvent() const {
  return outputReqQueue.canWriteEvent();
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

uint MemoryBank::memoryIndex() const {
  return parent().memoryIndex(id);
}

uint MemoryBank::memoriesThisTile() const {
  return parent().numMemories();
}

uint MemoryBank::globalMemoryIndex() const {
  return parent().globalMemoryIndex(id);
}

bool MemoryBank::isCore(ChannelID destination) const {
  return parent().isCore(destination.component);
}

uint MemoryBank::coresThisTile() const {
  return parent().numCores();
}

uint MemoryBank::globalCoreIndex(ComponentID core) const {
  loki_assert(parent().isCore(core));
  return parent().globalCoreIndex(core);
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

    hitRequest = peekRequest();

    if (hitUnderMiss && (missRequest != NULL)) {
      // Don't reorder data being sent to the same channel.
      if (missRequest->getDestination() == hitRequest->getDestination())
        return;

      // Don't start a request for the same location as the missing request.
      if (getTag(missRequest->getAddress()) == getTag(hitRequest->getAddress()))
        return;

      // Don't start another request if it will miss.
      if (!hitRequest->inCache())
        return;

      // ... or if it will flush data, as though it were a miss.
      if (hitRequest->getMetadata().opcode == FLUSH_LINE ||
          hitRequest->getMetadata().opcode == FLUSH_ALL_LINES)
        return;

      // Don't start a request which skips this cache, but will return data
      // through it.
      if (hitRequest->needsForwarding() && hitRequest->resultsToSend())
        return;

      // Don't start a second request until we have all payload flits from a
      // core. It's possible that the payload depends on the missing request.
      // +1 because we haven't dequeued the head flit yet.
      // (This differs from the Verilog which has two separate request
      // handlers.)
      if (hitRequest->awaitingPayload() && isCore(hitRequest->getDestination())
          && (inputQueue.size() < 1+hitRequest->payloadFlitsRemaining()))
        return;

      LOKI_LOG << this->name() << " starting hit-under-miss" << endl;
    }

    consumeRequest(hitRequest->getMemoryLevel());
    state = STATE_REQUEST;
    next_trigger(sc_core::SC_ZERO_TIME);

    LOKI_LOG << this->name() << " starting " << memoryOpName(hitRequest->getMetadata().opcode)
        << " request from component " << hitRequest->getDestination().component << endl;
  }
  // Nothing to do - wait for input to arrive.
  else {
    next_trigger(responseAvailableEvent() | requestAvailableEvent());
  }
}

void MemoryBank::processRequest(DecodedRequest& request) {
  loki_assert_with_message(state == STATE_REQUEST, "State = %d", state);
  loki_assert(request != NULL);

  // Before we even consider serving a request, we must make sure that there
  // is space to buffer any potential results.

  if (!canSendRequest()) {
    LOKI_LOG << this->name() << " delayed request due to full output request queue" << endl;
    next_trigger(canSendRequestEvent());
  }
  else if (request->needsForwarding()) {
    forwardRequest(request->getOriginal());
    if (request->awaitingPayload())
      state = STATE_FORWARD;
    else {
      loki_assert(missRequest == NULL);
      missRequest = request;
      finishedRequestForNow(request);
    }
  }
  else if (!request->preconditionsMet()) {
    request->prepare();
  }
  else if (request->resultsToSend() && !canSendResponse(request->getDestination(), request->getMemoryLevel())) {
    LOKI_LOG << this->name() << " delayed request due to full output queue" << endl;
    next_trigger(canSendResponseEvent(request->getDestination(), request->getMemoryLevel()));
  }
  else if (!iClock.negedge()) {
    next_trigger(iClock.negedge_event());
  }
  else if (!request->complete()) {
    request->execute();
  }

  // If the operation has finished, and we haven't moved to some other state
  // for further processing, end the request and prepare for a new one.
  if (request != NULL && request->complete() && state == STATE_REQUEST) {
    finishedRequest(request);
    readingFromMissBuffer = false;
  }
}

void MemoryBank::processAllocate(DecodedRequest& request) {
  loki_assert_with_message(state == STATE_ALLOCATE, "State = %d", state);
  loki_assert(request != NULL);

  if (!canSendRequest()) {
    LOKI_LOG << this->name() << " delayed allocation due to full output request queue" << endl;
    next_trigger(canSendRequestEvent());
  }
  // Use inCache to check whether the line has already been allocated. The data
  // won't have arrived yet.
  else if (!request->inCache()) {
    // The request has already called allocate() above, so data for the new line
    // is on its way. Now we must prepare the line for the data's arrival.
    request->validateLine();

    // Put the request to one side until its data comes back.
    missRequest = request;
    missRequest->notifyCacheMiss();
    cacheMissEvent.notify(sc_core::SC_ZERO_TIME);

    if (missRequest->awaitingPayload())
      copyingToMissBuffer = true;

    if (state != STATE_FLUSH) {
      state = STATE_IDLE;
      request.reset();
    }
  }
  else {
    state = STATE_IDLE;
    request.reset();
  }
}

void MemoryBank::processFlush(DecodedRequest& request) {
  loki_assert_with_message(state == STATE_FLUSH, "State = %d", state);
  loki_assert(request != NULL);
  // It is assumed that the header flit has already been sent. All that is left
  // is to send the cache line.

  if (!iClock.negedge())
    next_trigger(iClock.negedge_event());
  else if (canSendRequest()) {
    SRAMAddress position = getTag(request->getSRAMAddress()) + cacheLineCursor;
    uint32_t data = readWord(position, request->getAccessMode());

    Instrumentation::L1Cache::continueOperation(*this, FLUSH_LINE,
        metadata[getLine(position)].address + cacheLineCursor, false, ChannelID(id,0));

    cacheLineCursor += BYTES_PER_WORD;

    // Flush has finished if the cursor has covered a whole cache line.
    bool endOfPacket = cacheLineCursor >= cacheLineSize();
    if (endOfPacket)
      state = previousState;

    NetworkRequest flit(data, id, PAYLOAD, endOfPacket);
    sendRequest(flit);
  }
  else
    next_trigger(canSendRequestEvent());
}

void MemoryBank::processRefill(DecodedRequest& request) {
  loki_assert_with_message(state == STATE_REFILL, "State = %d", state);
  loki_assert(request != NULL);

  if (!iClock.negedge())
    next_trigger(iClock.negedge_event());
  else if (responseAvailable()) {

    // Don't store data locally if the request bypasses this cache.
    if (request->needsForwarding()) {
      if (canSendResponse(request->getDestination(), request->getMemoryLevel())) {
        uint32_t data = getResponse();
        request->sendResult(data);

        if (request->resultsToSend()) {
          next_trigger(responseAvailableEvent());
        }
        else {
          state = STATE_IDLE;
          request.reset();
        }
      }
      else
        next_trigger(canSendResponseEvent(request->getDestination(), request->getMemoryLevel()));
    }
    else {
      uint32_t data = getResponse();
      SRAMAddress position = getTag(request->getSRAMAddress()) + cacheLineCursor;
      writeWord(position, data, request->getAccessMode());

      cacheLineCursor += BYTES_PER_WORD;

      // Refill has finished if the cursor has covered a whole cache line.
      if (cacheLineCursor >= cacheLineSize()) {
        state = STATE_REQUEST;
        hitRequest = request;
        request.reset();
        readingFromMissBuffer = true;

        // Storing the requested words will have dirtied the cache line, but it
        // is actually clean.
        metadata[getLine(position)].dirty = false;

        LOKI_LOG << this->name() << " resuming " << memoryOpName(hitRequest->getMetadata().opcode) << " request" << endl;
      }
      else
        next_trigger(responseAvailableEvent());
    }
  }
  else
    next_trigger(responseAvailableEvent());
}

void MemoryBank::processForward(DecodedRequest& request) {
  loki_assert_with_message(state == STATE_FORWARD, "State = %d", state);
  loki_assert(request != NULL);

  // Before we even consider serving a request, we must make sure that there
  // is space to buffer any potential results.

  if (!canSendRequest()) {
    LOKI_LOG << this->name() << " delayed request due to full output request queue" << endl;
    next_trigger(canSendRequestEvent());
  }
  else if (!payloadAvailable(request->getMemoryLevel())) {
    next_trigger(requestAvailableEvent());
  }
  else {
    NetworkRequest payload;
    switch (request->getMemoryLevel()) {
      case MEMORY_L1:
        payload = inputQueue.read();
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
      if (request->resultsToSend()) {
        loki_assert(missRequest == NULL);
        missRequest = request;
        finishedRequestForNow(request);
      }
      else
        finishedRequest(request);
    }
  }
}

void MemoryBank::finishedRequest(DecodedRequest& request) {
  finishedRequestForNow(request);
}

void MemoryBank::finishedRequestForNow(DecodedRequest& request) {
  state = STATE_IDLE;
  request.reset();

  // Decode the next request immediately so it is ready to start next cycle.
  next_trigger(sc_core::SC_ZERO_TIME);
}

bool MemoryBank::requestAvailable() const {
  return (inputQueue.dataAvailable() && !isPayload(inputQueue.peek()))
      || (requestSig.valid() && !isPayload(requestSig.read()));
}

const sc_event_or_list& MemoryBank::requestAvailableEvent() const {
  return inputQueue.dataAvailableEvent() | requestSig.default_event();
}

MemoryBank::DecodedRequest MemoryBank::peekRequest() {
  loki_assert(requestAvailable());

  NetworkRequest request;
  MemoryLevel level;
  ChannelID destination;

  if (inputQueue.dataAvailable() && !isPayload(inputQueue.peek())) {
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

  return DecodedRequest(decodeMemoryRequest(request, *this, level, destination));
}

void MemoryBank::consumeRequest(MemoryLevel level) {
  loki_assert(requestAvailable());

  switch (level) {
    case MEMORY_L1: {
      NetworkRequest request = inputQueue.read();
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
  return outputReqQueue.canWrite();
}

const sc_event& MemoryBank::canSendRequestEvent() const {
  return outputReqQueue.canWriteEvent();
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
  return iResponse.valid() && (iResponseTarget.read() == id.position-coresThisTile());
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
      if (destination.channel < Core::numInstructionChannels)
        return outputInstQueue.canWrite();
      else
        return outputDataQueue.canWrite();
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
      if (destination.channel < Core::numInstructionChannels)
        return outputInstQueue.canWriteEvent();
      else
        return outputDataQueue.canWriteEvent();
    case MEMORY_L2:
      return oResponse.ack_event();
    default:
      loki_assert_with_message(false, "Memory bank can't handle off-chip requests", 0);
      return outputDataQueue.canWriteEvent();
  }
}

void MemoryBank::sendResponse(NetworkResponse response, MemoryLevel level) {
  loki_assert(canSendResponse(response.channelID(), level));

  // Instrumentation for buffering the result is in MemoryOperation because
  // it uses extra information about the request.

  switch (level) {
    case MEMORY_L1:
      if (response.channelID().channel < Core::numInstructionChannels) {
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

    switch (missRequest->getMemoryLevel()) {
      case MEMORY_L1:
        dataAvailable = inputQueue.dataAvailable();
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

    switch (missRequest->getMemoryLevel()) {
      case MEMORY_L1:
        inputQueue.read();
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
  MemoryAddr globalAddress = chip().getAddressTranslation(id.tile, operation.getAddress());
  bool scratchpad = operation.getAccessMode() == MEMORY_SCRATCHPAD;
  bool inMainMemory = !scratchpad && chip().backedByMainMemory(id.tile, operation.getAddress());
  if (inMainMemory && mainMemory->readOnly(globalAddress) && WARN_READ_ONLY) {
    LOKI_WARN << this->name() << " attempting to modify read-only address" << endl;
    LOKI_WARN << "  " << operation.toString() << endl;
  }
}

// Instrumentation only.
void MemoryBank::coreRequestArrived() {
  Instrumentation::Latency::memoryReceivedRequest(id, inputQueue.lastDataWritten());
}

// Instrumentation only.
void MemoryBank::coreDataSent() {
  Flit<Word> flit = outputDataQueue.lastDataRead();
  if (ENERGY_TRACE)
    Instrumentation::Network::traffic(id, flit.channelID().component);
  Instrumentation::Latency::memorySentResult(id, flit, true);
}

// Instrumentation only.
void MemoryBank::coreInstructionSent() {
  Flit<Word> flit = outputInstQueue.lastDataRead();
  if (ENERGY_TRACE)
    Instrumentation::Network::traffic(id, flit.channelID().component);
  Instrumentation::Latency::memorySentResult(id, flit, true);
}

// Method which sends data from the mOutputReqQueue whenever possible.
void MemoryBank::handleRequestOutput() {
  if (!iClock.posedge())
    next_trigger(iClock.posedge_event());
  else if (!outputReqQueue.dataAvailable())
    next_trigger(outputReqQueue.dataAvailableEvent());
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
    case STATE_IDLE:      processIdle();                break;
    case STATE_REQUEST:   processRequest(hitRequest);   break;
    case STATE_ALLOCATE:  processAllocate(hitRequest);  break;
    case STATE_FLUSH:     processFlush(hitRequest);     break;
    case STATE_REFILL:    processRefill(missRequest);   break;
    case STATE_FORWARD:   processForward(hitRequest);   break;
  }
}

void MemoryBank::updateIdle() {
  bool wasIdle = currentlyIdle;
  currentlyIdle = state == STATE_IDLE &&
                  !inputQueue.dataAvailable() && !outputDataQueue.dataAvailable() &&
                  !outputInstQueue.dataAvailable() && !outputReqQueue.dataAvailable();

  if (wasIdle != currentlyIdle)
    Instrumentation::idle(id, currentlyIdle);
}

cycle_count_t MemoryBank::artificialDelayRequired(const memory_bank_parameters_t& params) {
  // The networks to/from memory take some time, and the memory bank itself
  // currently has a minimum latency of 1 cycle.
  return params.latency - EXTERNAL_LATENCY - 1;
}

size_t MemoryBank::requestQueueSize(const memory_bank_parameters_t& params) {
  // The worst case is when we need to flush a cache line to make space for a
  // new one. This requires:
  //  * 1 flit to request new line
  //  * 1 flit header for old line
  //  * Many flits of data
  return 2 + params.cacheLineSize/BYTES_PER_WORD;
}

ComputeTile& MemoryBank::parent() const {
  return static_cast<ComputeTile&>(*(this->get_parent_object()));
}

Chip& MemoryBank::chip() const {
  return parent().chip();
}

MemoryBank::MemoryBank(sc_module_name name, const ComponentID& ID, uint numBanks,
                       const memory_bank_parameters_t& params, uint numCores) :
  MemoryBase(name, ID, params.log2CacheLineSize()),
  iClock("iClock"),
  iData("iData"),
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
  hitUnderMiss(params.hitUnderMiss),
  log2NumBanks(log2(numBanks)),
  inputQueue("mInputQueue", params.inputFIFO.size),
  outputDataQueue("mOutputDataQueue", params.outputFIFO.size, artificialDelayRequired(params)),
  outputInstQueue("mOutputInstQueue", params.outputFIFO.size, artificialDelayRequired(params)),
  outputReqQueue("mOutputReqQueue", requestQueueSize(params), 0),
  data(params.size/BYTES_PER_WORD, 0),
  metadata(params.size/params.cacheLineSize),
  reservations(1),
  missBuffer("mMissBuffer", params.cacheLineSize/BYTES_PER_WORD),
  cacheMissEvent(sc_core::sc_gen_unique_name("mCacheMissEvent")),
  l2RequestFilter("request_filter", *this)
{
  state = STATE_IDLE;
  previousState = STATE_IDLE;

  cacheLineCursor = 0;

  copyingToMissBuffer = false;
  readingFromMissBuffer = false;

  // Magic interfaces.
  mainMemory = NULL;

  // Connect to local components.
  iData(inputQueue);
  oData(outputDataQueue);
  oInstruction(outputInstQueue);

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

  for (uint line=0; line<metadata.size(); line++) {
    metadata[line].valid = false;
  }

  SC_METHOD(mainLoop);
  sensitive << iClock.neg();
  dont_initialize();

  SC_METHOD(updateIdle);
  sensitive << inputQueue.canWriteEvent() << inputQueue.dataAvailableEvent()
            << outputDataQueue.canWriteEvent() << outputDataQueue.dataAvailableEvent()
            << outputInstQueue.canWriteEvent() << outputInstQueue.dataAvailableEvent()
            << outputReqQueue.canWriteEvent() << outputReqQueue.dataAvailableEvent()
            << iResponse << oResponse;
  // do initialise

  SC_METHOD(coreDataSent);
  SC_METHOD(coreInstructionSent);
  SC_METHOD(handleRequestOutput);

  SC_METHOD(copyToMissBuffer);
  sensitive << cacheMissEvent;
  dont_initialize();

  SC_METHOD(coreRequestArrived);
  sensitive << inputQueue.dataAvailableEvent();
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

  MemoryAddr address = (start / BYTES_PER_WORD) * BYTES_PER_WORD;
  MemoryAddr limit = (end / BYTES_PER_WORD) * BYTES_PER_WORD + BYTES_PER_WORD;

  while (address < limit) {
    uint32_t data = readWordDebug(address).toUInt();
    cout << "0x" << setprecision(8) << setfill('0') << hex << address << ":  " << "0x" << setprecision(8) << setfill('0') << hex << data << dec << endl;
    address += BYTES_PER_WORD;
  }
}

Word MemoryBank::readWordDebug(MemoryAddr addr) {
  loki_assert(mainMemory != NULL);
  loki_assert_with_message(addr % BYTES_PER_WORD == 0, "Address = 0x%x", addr);

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
  loki_assert_with_message(addr % BYTES_PER_WORD == 0, "Address = 0x%x", addr);

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
  if (!inputQueue.canWrite()) {
    os << inputQueue.name() << " is full." << endl;
  }
  if (outputDataQueue.dataAvailable()) {
    const NetworkResponse& outWord = outputDataQueue.peek();
    os << this->name() << " waiting to send " << outWord << endl;
  }
  if (outputInstQueue.dataAvailable()) {
    const NetworkResponse& outWord = outputInstQueue.peek();
    os << this->name() << " waiting to send " << outWord << endl;
  }
  if (outputReqQueue.dataAvailable()) {
    const NetworkResponse& outWord = outputReqQueue.peek();
    os << this->name() << " waiting to send " << outWord << endl;
  }
}
