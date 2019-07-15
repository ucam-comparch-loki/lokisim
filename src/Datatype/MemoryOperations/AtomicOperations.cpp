/*
 * AtomicOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "AtomicOperations.h"
#include "../../Tile/Memory/MemoryBank.h"

#include <assert.h>

AtomicOperation::AtomicOperation(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    LoadStoreOperation(request, memory, level, destination, 1, 1, BYTES_PER_WORD) {
  intermediateData = 0;
  preWriteCheck();
}

void AtomicOperation::execute() {
  assert(preconditionsMet());

  // First part: load original data and return to requester.
  if (resultFlits == 1) {
    intermediateData = memory.readWord(sramAddress, getAccessMode());
    sendResult(intermediateData);
  }
  // Second part: modify original data and store back.
  else if (payloadAvailable()) {
    unsigned int data = atomicUpdate(intermediateData, getPayload());

    memory.writeWord(sramAddress, data, getAccessMode());
    memory.printOperation(metadata.opcode, address, data);
  }
}


LoadLinked::LoadLinked(const NetworkRequest& request, MemoryBase& memory,
                       MemoryLevel level, ChannelID destination) :
    LoadWord(request, memory, level, destination) {
  // Nothing
}

void LoadLinked::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readWord(sramAddress, getAccessMode());
  memory.makeReservation(destination.component, address, getAccessMode());
  memory.printOperation(metadata.opcode, address, result);
  sendResult(result);
}


StoreConditional::StoreConditional(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    LoadStoreOperation(request, memory, level, destination, 1, 1, BYTES_PER_WORD) {
  success = false;
  preWriteCheck();
}

void StoreConditional::prepare() {
  // Nothing
}

bool StoreConditional::preconditionsMet() const {
  // Since the operation is allowed to fail, there are no preconditions.
  return true;
}

void StoreConditional::execute() {
  assert(preconditionsMet());

  // First part: check address and see if operation should continue.
  if (resultFlits == 1) {
    success = inCache()
           && memory.checkReservation(destination.component, address, getAccessMode());

    sendResult(success);
  }
  // Second part: consume the payload and store if necessary.
  else if (payloadAvailable()) {
    unsigned int data = getPayload();
    if (success) {
      memory.writeWord(sramAddress, data, getAccessMode());
      memory.printOperation(metadata.opcode, address, data);
    }
  }
}


LoadAndAdd::LoadAndAdd(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    AtomicOperation(request, memory, level, destination) {
  // Nothing
}

uint LoadAndAdd::atomicUpdate(uint original, uint update) {
  return original + update;
}


LoadAndOr::LoadAndOr(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    AtomicOperation(request, memory, level, destination) {
  // Nothing
}

uint LoadAndOr::atomicUpdate(uint original, uint update) {
  return original | update;
}


LoadAndAnd::LoadAndAnd(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    AtomicOperation(request, memory, level, destination) {
  // Nothing
}

uint LoadAndAnd::atomicUpdate(uint original, uint update) {
  return original & update;
}


LoadAndXor::LoadAndXor(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    AtomicOperation(request, memory, level, destination) {
  // Nothing
}

uint LoadAndXor::atomicUpdate(uint original, uint update) {
  return original ^ update;
}


Exchange::Exchange(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    AtomicOperation(request, memory, level, destination) {
  // Nothing
}

uint Exchange::atomicUpdate(uint original, uint update) {
  return update;
}
