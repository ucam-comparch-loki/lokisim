/*
 * IPKCache.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef IPKCACHE_H_
#define IPKCACHE_H_

#include "InstrumentationBase.h"

class IPKCache: public InstrumentationBase {

public:

  static void cacheHit(bool hit);
  static void printStats();

private:

  static int numHits, numMisses;

};

#endif /* IPKCACHE_H_ */
