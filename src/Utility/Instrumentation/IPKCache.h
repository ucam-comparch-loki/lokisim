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
#include "CounterMap.h"

namespace Instrumentation {

using std::vector;

class IPKCache: public InstrumentationBase {

public:

  static void reset();

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
  static void instructionPacketStats(std::ostream& os);
  static void dumpEventCounts(std::ostream& os);

private:

  struct CoreStats {
    count_t hits;
    count_t misses;
    count_t reads;
    count_t writes;
  };
  static vector<struct CoreStats> perCore;
  static struct CoreStats total;

  // Data for energy modelling - currently only interested in aggregate across
  // all cores.
  static count_t tagWriteHD_, tagWrites_, tagReadHD_;
  static count_t tagsActive_, dataActive_;

  // Count how many times each instruction packet was executed.
  static CounterMap<MemoryAddr> packetsExecuted;
};

}

#endif /* IPKCACHE_H_ */
