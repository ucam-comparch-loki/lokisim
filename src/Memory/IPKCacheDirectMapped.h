/*
 * IPKCacheDirectMapped.h
 *
 *  Created on: 11 Jul 2012
 *      Author: db434
 */

#ifndef IPKCACHEDIRECTMAPPED_H_
#define IPKCACHEDIRECTMAPPED_H_

#include "IPKCacheBase.h"

class IPKCacheDirectMapped: public IPKCacheBase {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  IPKCacheDirectMapped(const size_t size, const std::string& name);
  virtual ~IPKCacheDirectMapped();

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual CacheIndex cacheIndex(const MemoryAddr address) const;
  virtual void setTag(const CacheIndex position, const MemoryAddr tag);
  virtual void updateReadPointer();
  virtual void updateWritePointer();

};

#endif /* IPKCACHEDIRECTMAPPED_H_ */
