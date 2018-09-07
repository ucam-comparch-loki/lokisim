/*
 * IPKCacheDirectMapped.cpp
 *
 *  Created on: 11 Jul 2012
 *      Author: db434
 */

#include "IPKCacheDirectMapped.h"

IPKCacheDirectMapped::IPKCacheDirectMapped(const std::string& name,
    const size_t size, size_t maxIPKLength) :
    IPKCacheBase(name, size, size, maxIPKLength) {
  // Do nothing
}

IPKCacheDirectMapped::~IPKCacheDirectMapped() {
  // Do nothing
}

CacheIndex IPKCacheDirectMapped::cacheIndex(const MemoryAddr address) const {
  // There is only one possible place the data could be stored, so check there.
  CacheIndex expected = (address/BYTES_PER_WORD) % size();

  if (tags[expected] == address)
    return expected;
  else
    return NOT_IN_CACHE;
}

void IPKCacheDirectMapped::setTag(const CacheIndex position, const MemoryAddr tag) {
  tags[position] = tag;
}

void IPKCacheDirectMapped::updateReadPointer() {
  if (readPointer.isNull()) {
//    readPointer = currentPacket.cacheIndex;
  }
  else if (jumpAmount != 0) {
    readPointer += jumpAmount;
    jumpAmount = 0;
  }
  else
    incrementReadPos();
}

void IPKCacheDirectMapped::updateWritePointer() {
//  MemoryAddr memAddr;
//
//  if (!currentPacket.inCache)
//    memAddr = currentPacket.memAddr;
//  else
//    memAddr = pendingPacket.memAddr;
//
//  writePointer = (memAddr/BYTES_PER_WORD) % size();
}
