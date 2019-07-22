/*
 * BasicOperations.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: db434
 */

#include "BasicOperations.h"
#include "../../Tile/Memory/MemoryBank.h"

#include <assert.h>

LoadStoreOperation::LoadStoreOperation(MemoryAddr address,
                                       MemoryMetadata metadata,
                                       ChannelID returnAddress,
                                       MemoryData datatype,
                                       MemoryAlignment alignment,
                                       uint iterations,
                                       bool reads,
                                       bool writes) :
    MemoryOperation(address, metadata, returnAddress, datatype, alignment,
                    iterations, reads, writes) {
  // Nothing
}

bool LoadStoreOperation::preconditionsMet() const {
  return inCache();
}


LoadOperation::LoadOperation(MemoryAddr address,
                             MemoryMetadata metadata,
                             ChannelID returnAddress,
                             MemoryData datatype,
                             MemoryAlignment alignment,
                             uint iterations) :
    LoadStoreOperation(address, metadata, returnAddress, datatype, alignment,
                       iterations, true, false) {
  // Nothing
}

uint LoadOperation::payloadFlitsRemaining() const {
  return 0;
}

uint LoadOperation::resultFlitsRemaining() const {
  return totalIterations - iterationsComplete;
}

bool LoadOperation::oneIteration() {
  uint32_t result = readMemory();
  sendResult(result);
  return true;
}


StoreOperation::StoreOperation(MemoryAddr address,
                               MemoryMetadata metadata,
                               ChannelID returnAddress,
                               MemoryData datatype,
                               MemoryAlignment alignment,
                               uint iterations) :
    LoadStoreOperation(address, metadata, returnAddress, datatype, alignment,
                       iterations, false, true) {
  // Nothing
}

uint StoreOperation::payloadFlitsRemaining() const {
  return totalIterations - iterationsComplete;
}

uint StoreOperation::resultFlitsRemaining() const {
  return 0;
}

bool StoreOperation::oneIteration() {
  if (payloadAvailable()) {
    unsigned int data = getPayload();
    writeMemory(data);
    return true;
  }
  else
    return false;
}


LoadWord::LoadWord(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    LoadOperation(address, metadata, returnAddr, MEMORY_WORD, ALIGN_WORD, 1) {
  // Nothing
}


LoadHalfword::LoadHalfword(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    LoadOperation(address, metadata, returnAddr, MEMORY_HALFWORD, ALIGN_HALFWORD, 1) {
  // Nothing
}


LoadByte::LoadByte(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    LoadOperation(address, metadata, returnAddr, MEMORY_BYTE, ALIGN_BYTE, 1) {
  // Nothing
}


StoreWord::StoreWord(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    StoreOperation(address, metadata, returnAddr, MEMORY_WORD, ALIGN_WORD, 1) {
  // Nothing
}


StoreHalfword::StoreHalfword(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    StoreOperation(address, metadata, returnAddr, MEMORY_HALFWORD, ALIGN_HALFWORD, 1) {
  // Nothing
}


StoreByte::StoreByte(MemoryAddr address, MemoryMetadata metadata, ChannelID returnAddr) :
    StoreOperation(address, metadata, returnAddr, MEMORY_BYTE, ALIGN_BYTE, 1) {
  // Nothing
}
