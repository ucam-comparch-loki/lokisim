/*
 * IPKCacheFullyAssociative.cpp
 *
 *  Created on: 11 Jul 2012
 *      Author: db434
 */

#include "IPKCacheFullyAssociative.h"

#include "../Utility/Instrumentation/IPKCache.h"

IPKCacheFullyAssociative::IPKCacheFullyAssociative(const std::string& name,
                                                   size_t size,
                                                   size_t numTags,
                                                   size_t maxIPKLength) :
    IPKCacheBase(name, size, numTags, maxIPKLength),
    packetPointers(numTags, NOT_IN_CACHE),
    nextTag(numTags) {

}

CacheIndex IPKCacheFullyAssociative::write(const Instruction inst) {
  CacheIndex returnVal = IPKCacheBase::write(inst);

  // As well as writing the instruction, we also need to invalidate any tags
  // which point to the cache entry we're writing to.

  CacheIndex entry = writePointer.value();
  for (unsigned int i=0; i<packetPointers.size(); i++) {
    if (packetPointers[i] == entry)
      tags[i] = NOT_IN_CACHE;
  }

  return returnVal;
}

CacheIndex IPKCacheFullyAssociative::cacheIndex(const MemoryAddr address) const {
  // Check all tags for a match.
  for (CacheIndex i=0; i<tags.size(); i++) {
    if (tags[i] == address)
      return packetPointers[i];
  }

  // If we got this far, the tag wasn't found.
  return NOT_IN_CACHE;
}

void IPKCacheFullyAssociative::setTag(const CacheIndex position, const MemoryAddr tag) {
  TagIndex tagIndex = ++nextTag;

  // FIXME: currently setting whole tags to 0xFFFFFFFF instead of using a
  // single invalidation bit. This messes with the recorded Hamming distances.
  if (ENERGY_TRACE)
    Instrumentation::IPKCache::tagWrite(tags[tagIndex], tag);

  tags[tagIndex] = tag;
  packetPointers[tagIndex] = position;
}

// Move the read pointer to the next instruction. In the common case, this
// will just be an increment, but we could also perform an in-buffer jump or
// switch to a new instruction packet.
void IPKCacheFullyAssociative::updateReadPointer() {
  if (jumpAmount != 0) {
    readPointer += jumpAmount;
    jumpAmount = 0;
  }
  else
    incrementReadPos();
}

void IPKCacheFullyAssociative::updateWritePointer() {
  incrementWritePos();
}
