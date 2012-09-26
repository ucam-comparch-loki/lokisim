/*
 * IPKCacheDirectMapped.cpp
 *
 *  Created on: 11 Jul 2012
 *      Author: db434
 */

#include "IPKCacheDirectMapped.h"

IPKCacheDirectMapped::IPKCacheDirectMapped(const size_t size, const std::string& name) :
    IPKCacheBase(size, size, name) {
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

MemoryAddr IPKCacheDirectMapped::getTag(const CacheIndex position) const {
  return tags[position];
}

void IPKCacheDirectMapped::setTag(const CacheIndex position, const MemoryAddr tag) {
  tags[position] = tag;
}

void IPKCacheDirectMapped::updateReadPointer() {
  if (readPointer.isNull()) {
//    readPointer = currentPacket.cacheIndex;
    updateFillCount();
  }
  else if (jumpAmount != 0) {
    readPointer += jumpAmount;
    jumpAmount = 0;
    updateFillCount();
  }
  else
    incrementReadPos();
}

void IPKCacheDirectMapped::updateWritePointer() {
  MemoryAddr memAddr;

//  if (!currentPacket.inCache)
//    memAddr = currentPacket.memAddr;
//  else
//    memAddr = pendingPacket.memAddr;

  writePointer = (memAddr/BYTES_PER_WORD) % size();
  updateFillCount();
}
