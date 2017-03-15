/*
 * AtomicOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "AtomicOperations.h"

#include <assert.h>

#include "../../Tile/Memory/MemoryBank.h"

LoadLinked::LoadLinked(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 0, 1, BYTES_PER_WORD) {
  // Nothing
}

void LoadLinked::prepare() {
  allocateLine();
}

bool LoadLinked::preconditionsMet() const {
  return inCache();
}

void LoadLinked::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readWord(sramAddress, getAccessMode());
  memory.makeReservation(destination.component, address, getAccessMode());
  memory.printOperation(metadata.opcode, address, result);
  sendResult(result);
}


StoreConditional::StoreConditional(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 1, 1, BYTES_PER_WORD) {
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
    MemoryOperation(request, memory, level, destination, 1, 1, BYTES_PER_WORD) {
  intermediateData = 0;
  preWriteCheck();
}

void LoadAndAdd::prepare() {
  allocateLine();
}

bool LoadAndAdd::preconditionsMet() const {
  return inCache();
}

void LoadAndAdd::execute() {
  assert(preconditionsMet());

  // First part: load original data and return to requester.
  if (resultFlits == 1) {
    intermediateData = memory.readWord(sramAddress, getAccessMode());
    sendResult(intermediateData);
  }
  // Second part: modify original data and store back.
  else if (payloadAvailable()) {
    unsigned int data = getPayload() + intermediateData;

    memory.writeWord(sramAddress, data, getAccessMode());
    memory.printOperation(metadata.opcode, address, data);
  }
}


LoadAndOr::LoadAndOr(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 1, 1, BYTES_PER_WORD) {
  intermediateData = 0;
  preWriteCheck();
}

void LoadAndOr::prepare() {
  allocateLine();
}

bool LoadAndOr::preconditionsMet() const {
  return inCache();
}

void LoadAndOr::execute() {
  assert(preconditionsMet());

  // First part: load original data and return to requester.
  if (resultFlits == 1) {
    intermediateData = memory.readWord(sramAddress, getAccessMode());
    sendResult(intermediateData);
  }
  // Second part: modify original data and store back.
  else if (payloadAvailable()) {
    unsigned int data = getPayload() | intermediateData;
    memory.writeWord(sramAddress, data, getAccessMode());
    memory.printOperation(metadata.opcode, address, data);
  }
}


LoadAndAnd::LoadAndAnd(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 1, 1, BYTES_PER_WORD) {
  intermediateData = 0;
  preWriteCheck();
}

void LoadAndAnd::prepare() {
  allocateLine();
}

bool LoadAndAnd::preconditionsMet() const {
  return inCache();
}

void LoadAndAnd::execute() {
  assert(preconditionsMet());

  // First part: load original data and return to requester.
  if (resultFlits == 1) {
    intermediateData = memory.readWord(sramAddress, getAccessMode());
    sendResult(intermediateData);
  }
  // Second part: modify original data and store back.
  else if (payloadAvailable()) {
    unsigned int data = getPayload() & intermediateData;
    memory.writeWord(sramAddress, data, getAccessMode());
    memory.printOperation(metadata.opcode, address, data);
  }
}


LoadAndXor::LoadAndXor(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 1, 1, BYTES_PER_WORD) {
  intermediateData = 0;
  preWriteCheck();
}

void LoadAndXor::prepare() {
  allocateLine();
}

bool LoadAndXor::preconditionsMet() const {
  return inCache();
}

void LoadAndXor::execute() {
  assert(preconditionsMet());

  // First part: load original data and return to requester.
  if (resultFlits == 1) {
    intermediateData = memory.readWord(sramAddress, getAccessMode());
    sendResult(intermediateData);
  }
  // Second part: modify original data and store back.
  else if (payloadAvailable()) {
    unsigned int data = getPayload() ^ intermediateData;
    memory.writeWord(sramAddress, data, getAccessMode());
    memory.printOperation(metadata.opcode, address, data);
  }
}


Exchange::Exchange(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 1, 1, BYTES_PER_WORD) {
  preWriteCheck();
}

void Exchange::prepare() {
  allocateLine();
}

bool Exchange::preconditionsMet() const {
  return inCache();
}

void Exchange::execute() {
  assert(preconditionsMet());

  // First part: load original data and return to requester.
  if (resultFlits == 1) {
    unsigned int data = memory.readWord(sramAddress, getAccessMode());
    sendResult(data);
  }
  // Second part: store new data.
  else if (payloadAvailable()) {
    unsigned int data = getPayload();
    memory.writeWord(sramAddress, data, getAccessMode());
    memory.printOperation(metadata.opcode, address, data);
  }
}
