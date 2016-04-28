/*
 * BasicOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "BasicOperations.h"

#include <assert.h>

#include "../MemoryBank.h"

LoadWord::LoadWord(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(address, metadata, memory, level, destination, 0, 1) {
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
  unsigned int result = memory.readWord(sramAddress);
  memory.printOperation(metadata.opcode, address, result);
  sendResult(result);
}


LoadHalfword::LoadHalfword(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(address, metadata, memory, level, destination, 0, 1) {
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
  unsigned int result = memory.readHalfword(sramAddress);
  memory.printOperation(metadata.opcode, address, result);
  sendResult(result);
}


LoadByte::LoadByte(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(address, metadata, memory, level, destination, 0, 1) {
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
  unsigned int result = memory.readByte(sramAddress);
  memory.printOperation(metadata.opcode, address, result);
  sendResult(result);
}


StoreWord::StoreWord(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(address, metadata, memory, level, destination, 1, 0) {
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
    memory.writeWord(sramAddress, data);
    memory.printOperation(metadata.opcode, address, data);
  }
}


StoreHalfword::StoreHalfword(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(address, metadata, memory, level, destination, 1, 0) {
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
    memory.writeHalfword(sramAddress, data);
    memory.printOperation(metadata.opcode, address, data);
  }
}


StoreByte::StoreByte(MemoryAddr address, MemoryMetadata metadata, MemoryBase& memory, MemoryLevel level, ChannelID destination) :
    MemoryOperation(address, metadata, memory, level, destination, 1, 0) {
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
    memory.writeByte(sramAddress, data);
    memory.printOperation(metadata.opcode, address, data);
  }
}
