/*
 * IPKCacheFullyAssociative.h
 *
 *  Created on: 11 Jul 2012
 *      Author: db434
 */

#ifndef IPKCACHEFULLYASSOCIATIVE_H_
#define IPKCACHEFULLYASSOCIATIVE_H_

#include "IPKCacheBase.h"

class IPKCacheFullyAssociative: public IPKCacheBase {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  IPKCacheFullyAssociative(const std::string& name, const size_t size, const size_t numTags);

//============================================================================//
// Methods
//============================================================================//

public:

  // Writes new data to the cache. The position to write to, and the tag for
  // the data, are already known. Returns the position written to.
  virtual CacheIndex write(const Instruction inst);

protected:

  virtual CacheIndex cacheIndex(const MemoryAddr address) const;
  virtual void setTag(const CacheIndex position, const MemoryAddr tag);
  virtual void updateReadPointer();
  virtual void updateWritePointer();

//============================================================================//
// Local state
//============================================================================//

private:

  // For a fully-associative cache, if we have a tag match, we need to follow
  // a pointer to see where in the cache the packet is.
  std::vector<CacheIndex> packetPointers;

  // Write to tags in a round-robin fashion.
  LoopCounter nextTag;

};

#endif /* IPKCACHEFULLYASSOCIATIVE_H_ */
