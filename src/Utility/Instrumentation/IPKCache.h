/*
 * IPKCache.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef IPKCACHE_H_
#define IPKCACHE_H_

#include "InstrumentationBase.h"

#include "../../Datatype/ComponentID.h"

namespace Instrumentation {

class IPKCache: public InstrumentationBase {

public:

  static void tagCheck(const ComponentID& core, bool hit);
  static void read(const ComponentID& core);
  static void write(const ComponentID& core);

  static unsigned long long  numTagChecks();
  static unsigned long long  numHits();
  static unsigned long long  numMisses();
  static unsigned long long  numReads();
  static unsigned long long  numWrites();

  static void printStats();

private:

  static unsigned long long numHits_, numMisses_, numReads_, numWrites_;

};

}

#endif /* IPKCACHE_H_ */
