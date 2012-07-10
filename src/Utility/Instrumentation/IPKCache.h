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
  static void tagWrite(const MemoryAddr oldTag, const MemoryAddr newTag);
  static void read(const ComponentID& core);
  static void write(const ComponentID& core);

  static count_t numTagChecks();
  static count_t numHits();
  static count_t numMisses();
  static count_t numReads();
  static count_t numWrites();

  static void printStats();
  static void dumpEventCounts(std::ostream& os);

private:

  static count_t numHits_, numMisses_, numReads_, numWrites_, tagHD_;

};

}

#endif /* IPKCACHE_H_ */
