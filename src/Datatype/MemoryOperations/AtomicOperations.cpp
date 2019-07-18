/*
 * AtomicOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "AtomicOperations.h"
#include "../../Tile/Memory/MemoryBank.h"

#include <assert.h>

AtomicOperation::AtomicOperation(MemoryAddr address,
                                 MemoryMetadata metadata,
                                 ChannelID returnAddress,
                                 MemoryData datatype,
                                 MemoryAlignment alignment,
                                 uint iterations) :
    LoadStoreOperation(address, metadata, returnAddress, datatype, alignment, iterations, true, true) {
  intermediateData = 0;
  readState = true;
}

uint AtomicOperation::payloadFlitsRemaining() const {
  return totalIterations - iterationsComplete;
}

uint AtomicOperation::resultFlitsRemaining() const {
  // There is a result to send in the read stage of each iteration.
  return totalIterations - iterationsComplete - (readState ? 0 : 1);
}

bool AtomicOperation::oneIteration() {
  // First part: load original data and return to requester.
  if (readState) {
    intermediateData = readMemory();
    sendResult(intermediateData);
    readState = false;
    return false; // The operation isn't finished yet.
  }
  // Second part: modify original data and store back.
  else if (payloadAvailable()) {
    uint data = atomicUpdate(intermediateData, getPayload());
    writeMemory(data);
    readState = true;
    return true;
  }
  else
    return false;
}


LoadLinked::LoadLinked(const NetworkRequest& request, ChannelID destination) :
    LoadWord(request, destination) {
  // Nothing
}

bool LoadLinked::oneIteration() {
  memory->makeReservation(returnAddress.component, address+addressOffset,
                          getAccessMode());
  return LoadWord::oneIteration();
}


StoreConditional::StoreConditional(const NetworkRequest& request, ChannelID destination) :
    LoadStoreOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                       destination, MEMORY_WORD, ALIGN_WORD, 1, false, true) {
  success = false;
  checkState = true;
}

void StoreConditional::prepare() {
  // Nothing - if the data is not already in the cache, the operation fails.
}

bool StoreConditional::preconditionsMet() const {
  // Since the operation is allowed to fail, there are no preconditions.
  return true;
}

uint StoreConditional::payloadFlitsRemaining() const {
  return totalIterations - iterationsComplete;
}

uint StoreConditional::resultFlitsRemaining() const {
  // There is a result to send in the check stage of each iteration.
  return totalIterations - iterationsComplete - (checkState ? 0 : 1);
}

bool StoreConditional::oneIteration() {
  // First part: check address and see if operation should continue.
  if (checkState) {
    success = inCache()
           && memory->checkReservation(returnAddress.component, address+addressOffset, getAccessMode());

    sendResult(success);

    checkState = false;
    return false; // Operation isn't finished yet
  }
  // Second part: consume the payload and store if necessary.
  else if (payloadAvailable()) {
    unsigned int data = getPayload();
    if (success)
      writeMemory(data);

    checkState = true;
    return true;
  }
  else
    return false;
}


LoadAndAdd::LoadAndAdd(const NetworkRequest& request, ChannelID destination) :
    AtomicOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                    destination, MEMORY_WORD, ALIGN_WORD, 1) {
  // Nothing
}

uint LoadAndAdd::atomicUpdate(uint original, uint update) {
  return original + update;
}


LoadAndOr::LoadAndOr(const NetworkRequest& request, ChannelID destination) :
    AtomicOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                    destination, MEMORY_WORD, ALIGN_WORD, 1) {
  // Nothing
}

uint LoadAndOr::atomicUpdate(uint original, uint update) {
  return original | update;
}


LoadAndAnd::LoadAndAnd(const NetworkRequest& request, ChannelID destination) :
    AtomicOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                    destination, MEMORY_WORD, ALIGN_WORD, 1) {
  // Nothing
}

uint LoadAndAnd::atomicUpdate(uint original, uint update) {
  return original & update;
}


LoadAndXor::LoadAndXor(const NetworkRequest& request, ChannelID destination) :
    AtomicOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                    destination, MEMORY_WORD, ALIGN_WORD, 1) {
  // Nothing
}

uint LoadAndXor::atomicUpdate(uint original, uint update) {
  return original ^ update;
}


Exchange::Exchange(const NetworkRequest& request, ChannelID destination) :
    AtomicOperation(request.payload().toUInt(), request.getMemoryMetadata(),
                    destination, MEMORY_WORD, ALIGN_WORD, 1) {
  // Nothing
}

uint Exchange::atomicUpdate(uint original, uint update) {
  return update;
}
