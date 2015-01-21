/*
 * IPKCache.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef IPKCACHE_H_
#define IPKCACHE_H_

#include "InstrumentationBase.h"

#include "../../Datatype/Identifier.h"

namespace Instrumentation {

class IPKCache: public InstrumentationBase {

public:

  static void tagCheck(const ComponentID& core, bool hit, const MemoryAddr tag, const MemoryAddr prevCheck);
  static void tagWrite(const MemoryAddr oldTag, const MemoryAddr newTag);
  static void tagActivity();

  static void read(const ComponentID& core);
  static void write(const ComponentID& core);
  static void dataActivity();

  static count_t numTagChecks();
  static count_t numHits();
  static count_t numMisses();
  static count_t numReads();
  static count_t numWrites();

  static void printStats();
  static void printSummary();
  static void dumpEventCounts(std::ostream& os);

private:

  static count_t numHits_, numMisses_, numReads_, numWrites_, tagWriteHD_, tagWrites_, tagReadHD_;
  static count_t tagsActive_, dataActive_;

};

}

#endif /* IPKCACHE_H_ */
