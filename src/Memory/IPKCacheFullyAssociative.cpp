/*
 * IPKCacheFullyAssociative.cpp
 *
 *  Created on: 11 Jul 2012
 *      Author: db434
 */

#include "IPKCacheFullyAssociative.h"

IPKCacheFullyAssociative::IPKCacheFullyAssociative(const size_t size,
                                                   const size_t numTags,
                                                   const std::string& name) :
    IPKCacheBase(size, numTags, name),
    alignment(size/numTags) {

  // Do nothing

}

CacheIndex IPKCacheFullyAssociative::cacheIndex(const MemoryAddr address) const {
  // Check all tags for a match.
  for (CacheIndex i=0; i<tags.size(); i++) {
    if (tags[i] == address)
      return i*alignment;
  }

  // If we got this far, the tag wasn't found.
  return NOT_IN_CACHE;
}

MemoryAddr IPKCacheFullyAssociative::getTag(const CacheIndex position) const {
  assert((position % alignment) == 0);
  return tags[position/alignment];
}

void IPKCacheFullyAssociative::setTag(const CacheIndex position, const MemoryAddr tag) {
  // Only do anything if the cache entry actually has a tag.
  if ((position % alignment) == 0) {
    TagIndex tagIndex = position/alignment;
    tags[tagIndex] = tag;
  }
}

// Move the read pointer to the next instruction. In the common case, this
// will just be an increment, but we could also perform an in-buffer jump or
// switch to a new instruction packet.
void IPKCacheFullyAssociative::updateReadPointer() {
  if (jumpAmount != 0) {
    readPointer += jumpAmount;
    jumpAmount = 0;
    updateFillCount();
  }
  else
    incrementReadPos();
}

void IPKCacheFullyAssociative::updateWritePointer() {
  incrementWritePos();

  // If we are about to start a new instruction packet, make sure it is aligned
  // with the next tag.
  if (finishedPacketWrite && (writePointer.value() % alignment) != 0) {
    writePointer += alignment - (writePointer.value() % alignment);
    updateFillCount();
  }
}
