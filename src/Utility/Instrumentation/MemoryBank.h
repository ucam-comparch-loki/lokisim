/*
 * MemoryBank.h
 *
 *  Created on: 19 Apr 2016
 *      Author: db434
 */

#ifndef SRC_UTILITY_INSTRUMENTATION_MEMORYBANK_H_
#define SRC_UTILITY_INSTRUMENTATION_MEMORYBANK_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"

using std::vector;

namespace Instrumentation {

  class MemoryBank : public InstrumentationBase {

  public:

    static void reset();

    static void startOperation(ComponentID bank, MemoryOpcode op,
        MemoryAddr address, bool miss, ChannelID returnChannel);
    static void continueOperation(ComponentID bank, MemoryOpcode op,
        MemoryAddr address, bool miss, ChannelID returnChannel);

    static void checkTags(ComponentID bank, MemoryAddr address);
    static void replaceCacheLine(ComponentID bank, bool isValid, bool isDirty);

    static void updateCoreStats(ChannelID returnChannel, MemoryOpcode op, bool miss);

    static void printSummary();
    static void dumpEventCounts(std::ostream& os);

    // Some very crude access methods to give energy estimation some concept
    // of memories.
    static count_t totalReads();
    static count_t totalWrites();

    static count_t totalWordReads();
    static count_t totalHalfwordReads();
    static count_t totalByteReads();
    static count_t totalBurstReads();
    static count_t totalWordWrites();
    static count_t totalHalfwordWrites();
    static count_t totalByteWrites();
    static count_t totalBurstWrites();
    static count_t totalLineReplacements();

    static count_t numIPKReadMisses();

  private:

    // Counters for various events. Index each CounterMap using the memory
    // bank's global bank number.

    static CounterMap<ComponentID>          tagChecks;

    // A vector of counters, one for each operation. Index using the opcode.
    static vector<CounterMap<ComponentID> > hits;
    static vector<CounterMap<ComponentID> > misses;

    // The above counters keep track of memory access at the level of individual
    // words. These counters track how many multi-word operations take place.
    static CounterMap<ComponentID>          ipkReads;
    static CounterMap<ComponentID>          burstReads;
    static CounterMap<ComponentID>          burstWrites;

    // Cache line replacement stats.
    static CounterMap<ComponentID>          replaceInvalidLine;
    static CounterMap<ComponentID>          replaceCleanLine;
    static CounterMap<ComponentID>          replaceDirtyLine;

    // Stats stored from the perspective of each input channel of each core.
    // It would make more sense to use output channels (input channels don't
    // write any data), but we do not have this information at the memory bank.
    struct ChannelStats {
      count_t readHits;
      count_t readMisses;
      count_t writeHits;
      count_t writeMisses;
      bool    receivingInstructions;
    };
    // Address using coreStats[core][channel].
    static vector<vector<struct ChannelStats> > coreStats;

  };

}

#endif /* SRC_UTILITY_INSTRUMENTATION_MEMORYBANK_H_ */
