/*
 * IPKCache.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef IPKCACHE_H_
#define IPKCACHE_H_

#include "InstrumentationBase.h"

namespace Instrumentation {

class IPKCache: public InstrumentationBase {

public:

  static void tagCheck(ComponentID core, bool hit);
  static void read(ComponentID core);
  static void write(ComponentID core);

  static int  numTagChecks();
  static int  numHits();
  static int  numMisses();
  static int  numReads();
  static int  numWrites();

  static void printStats();

private:

  static int numHits_, numMisses_, numReads_, numWrites_;

};

}

#endif /* IPKCACHE_H_ */
