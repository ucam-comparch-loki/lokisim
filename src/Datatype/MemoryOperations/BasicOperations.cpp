/*
 * BasicOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "BasicOperations.h"
#include "../../Tile/Memory/MemoryBank.h"

#include <assert.h>

LoadStoreOperation::LoadStoreOperation(const NetworkRequest& request,
                                       MemoryBase& memory,
                                       MemoryLevel level,
                                       ChannelID destination,
                                       unsigned int payloadFlits,
                                       unsigned int maxResultFlits,
                                       unsigned int alignment) :
    MemoryOperation(request, memory, level, destination, payloadFlits, maxResultFlits, alignment) {
  // Nothing
}

void LoadStoreOperation::prepare() {
  allocateLine();
}

bool LoadStoreOperation::preconditionsMet() const {
  return inCache();
}


LoadOperation::LoadOperation(const NetworkRequest& request, MemoryBase& memory,
                             MemoryLevel level, ChannelID destination,
                             unsigned int alignment) :
    LoadStoreOperation(request, memory, level, destination, 0, 1, alignment) {
  // Nothing
}


StoreOperation::StoreOperation(const NetworkRequest& request, MemoryBase& memory,
                               MemoryLevel level, ChannelID destination,
                               unsigned int alignment) :
    LoadStoreOperation(request, memory, level, destination, 1, 0, alignment) {

  preWriteCheck();

}


LoadWord::LoadWord(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    LoadOperation(request, memory, level, destination, BYTES_PER_WORD) {
  // Nothing
}

void LoadWord::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readWord(sramAddress, getAccessMode());
  memory.printOperation(metadata.opcode, address, result);
  sendResult(result);
}


LoadHalfword::LoadHalfword(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    LoadOperation(request, memory, level, destination, BYTES_PER_WORD/2) {
  // Nothing
}

void LoadHalfword::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readHalfword(sramAddress, getAccessMode());
  memory.printOperation(metadata.opcode, address, result);
  sendResult(result);
}


LoadByte::LoadByte(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    LoadOperation(request, memory, level, destination, 1) {
  // Nothing
}

void LoadByte::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readByte(sramAddress, getAccessMode());
  memory.printOperation(metadata.opcode, address, result);
  sendResult(result);
}


StoreWord::StoreWord(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    StoreOperation(request, memory, level, destination, BYTES_PER_WORD) {
  // Nothing
}

void StoreWord::execute() {
  assert(preconditionsMet());
  if (payloadAvailable()) {
    unsigned int data = getPayload();
    memory.writeWord(sramAddress, data, getAccessMode());
    memory.printOperation(metadata.opcode, address, data);
  }
}


StoreHalfword::StoreHalfword(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    StoreOperation(request, memory, level, destination, BYTES_PER_WORD/2) {
  // Nothing
}

void StoreHalfword::execute() {
  assert(preconditionsMet());
  if (payloadAvailable()) {
    unsigned int data = getPayload();
    memory.writeHalfword(sramAddress, data, getAccessMode());
    memory.printOperation(metadata.opcode, address, data);
  }
}


StoreByte::StoreByte(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    StoreOperation(request, memory, level, destination, 1) {
  // Nothing
}

void StoreByte::execute() {
  assert(preconditionsMet());
  if (payloadAvailable()) {
    unsigned int data = getPayload();
    memory.writeByte(sramAddress, data, getAccessMode());
    memory.printOperation(metadata.opcode, address, data);
  }
}
