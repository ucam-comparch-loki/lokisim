/*
 * BasicOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "BasicOperations.h"

#include <assert.h>

#include "../../Tile/Memory/MemoryBank.h"

LoadWord::LoadWord(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 0, 1, BYTES_PER_WORD) {
  // Nothing
}

void LoadWord::prepare() {
  allocateLine();
}

bool LoadWord::preconditionsMet() const {
  return inCache();
}

void LoadWord::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readWord(sramAddress, getAccessMode());
  memory.printOperation(metadata.opcode, address, result);
  sendResult(result);
}


LoadHalfword::LoadHalfword(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 0, 1, BYTES_PER_WORD/2) {
  // Nothing
}

void LoadHalfword::prepare() {
  allocateLine();
}

bool LoadHalfword::preconditionsMet() const {
  return inCache();
}

void LoadHalfword::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readHalfword(sramAddress, getAccessMode());
  memory.printOperation(metadata.opcode, address, result);
  sendResult(result);
}


LoadByte::LoadByte(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 0, 1, 1) {
  // Nothing
}

void LoadByte::prepare() {
  allocateLine();
}

bool LoadByte::preconditionsMet() const {
  return inCache();
}

void LoadByte::execute() {
  assert(preconditionsMet());
  unsigned int result = memory.readByte(sramAddress, getAccessMode());
  memory.printOperation(metadata.opcode, address, result);
  sendResult(result);
}


StoreWord::StoreWord(const NetworkRequest& request, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(request, memory, level, destination, 1, 0, BYTES_PER_WORD) {
  preWriteCheck();
}

void StoreWord::prepare() {
  allocateLine();
}

bool StoreWord::preconditionsMet() const {
  return inCache();
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
    MemoryOperation(request, memory, level, destination, 1, 0, BYTES_PER_WORD/2) {
  preWriteCheck();
}

void StoreHalfword::prepare() {
  allocateLine();
}

bool StoreHalfword::preconditionsMet() const {
  return inCache();
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
    MemoryOperation(request, memory, level, destination, 1, 0, 1) {
  preWriteCheck();
}

void StoreByte::prepare() {
  allocateLine();
}

bool StoreByte::preconditionsMet() const {
  return inCache();
}

void StoreByte::execute() {
  assert(preconditionsMet());
  if (payloadAvailable()) {
    unsigned int data = getPayload();
    memory.writeByte(sramAddress, data, getAccessMode());
    memory.printOperation(metadata.opcode, address, data);
  }
}
