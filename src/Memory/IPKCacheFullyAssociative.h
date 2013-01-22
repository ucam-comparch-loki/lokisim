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

//==============================//
// Constructors and destructors
//==============================//

public:

  IPKCacheFullyAssociative(const size_t size, const size_t numTags, const std::string& name);

//==============================//
// Methods
//==============================//

protected:

  virtual CacheIndex cacheIndex(const MemoryAddr address) const;
  virtual MemoryAddr getTag(const CacheIndex position) const;
  virtual void setTag(const CacheIndex position, const MemoryAddr tag);
  virtual void updateReadPointer();
  virtual void updateWritePointer();

//==============================//
// Local state
//==============================//

private:

  // If there are fewer tags than cache entries, each packet must be aligned
  // to the next tag. Tags are "alignment" entries apart.
  typedef uint16_t Alignment;
  const Alignment alignment;

};

#endif /* IPKCACHEFULLYASSOCIATIVE_H_ */
