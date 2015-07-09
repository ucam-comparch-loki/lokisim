/*
 * MemoryBank.h
 *
 *  Created on: 5 May 2011
 *      Author: afjk2
 */

#ifndef MEMORYBANK_H_2_
#define MEMORYBANK_H_2_

#include "InstrumentationBase.h"
#include "CounterMap.h"

#include <map>

using std::vector;

namespace Instrumentation {

	class MemoryBank : public InstrumentationBase {

	public:

	  static void init();

		static void setMode(int bank, bool isCache, uint setCount, uint wayCount, uint lineSize);

		static void startOperation(int bank, MemoryOperation op,
		    MemoryAddr address, bool miss, ChannelID returnChannel);

		static void readWord(int bank, MemoryAddr address, bool isMiss, ChannelID returnChannel);
		static void readHalfWord(int bank, MemoryAddr address, bool isMiss, ChannelID returnChannel);
		static void readByte(int bank, MemoryAddr address, bool isMiss, ChannelID returnChannel);

		static void writeWord(int bank, MemoryAddr address, bool isMiss, ChannelID returnChannel);
		static void writeHalfWord(int bank, MemoryAddr address, bool isMiss, ChannelID returnChannel);
		static void writeByte(int bank, MemoryAddr address, bool isMiss, ChannelID returnChannel);

		static void initiateIPKRead(int bank);
		static void initiateBurstRead(int bank);
		static void initiateBurstWrite(int bank);

		static void readIPKWord(int bank, MemoryAddr address, bool isMiss, ChannelID returnChannel);
		static void readBurstWord(int bank, MemoryAddr address, bool isMiss, ChannelID returnChannel);
		static void writeBurstWord(int bank, MemoryAddr address, bool isMiss, ChannelID returnChannel);

		static void replaceCacheLine(int bank, bool isValid, bool isDirty);

		static void updateCoreStats(ChannelID returnChannel, bool isRead, bool isInst, bool isMiss);

		static void printStats();
		static void printSummary();
	  static void dumpEventCounts(std::ostream& os);

		// Some very crude access methods to give energy estimation some concept
		// of memories.
		static long long numReads();
		static long long numWrites();

		static long long numIPKReadMisses();

	private:

		// Stats stored from the perspective of each memory bank.
		static std::map<int, bool> modes_;
		static std::map<int, uint> setCounts_;
		static std::map<int, uint> wayCounts_;
		static std::map<int, uint> lineSizes_;

		static CounterMap<int> numReadWordHits_;
		static CounterMap<int> numReadHalfWordHits_;
		static CounterMap<int> numReadByteHits_;

		static CounterMap<int> numWriteWordHits_;
		static CounterMap<int> numWriteHalfWordHits_;
		static CounterMap<int> numWriteByteHits_;

		static CounterMap<int> numReadWordMisses_;
		static CounterMap<int> numReadHalfWordMisses_;
		static CounterMap<int> numReadByteMisses_;

		static CounterMap<int> numWriteWordMisses_;
		static CounterMap<int> numWriteHalfWordMisses_;
		static CounterMap<int> numWriteByteMisses_;

		static CounterMap<int> numStartIPKRead_;
		static CounterMap<int> numStartBurstRead_;
		static CounterMap<int> numStartBurstWrite_;

		static CounterMap<int> numContinueIPKRead_;
		static CounterMap<int> numContinueBurstRead_;
		static CounterMap<int> numContinueBurstWrite_;

		static CounterMap<int> numIPKReadHits_;
		static CounterMap<int> numBurstReadHits_;
		static CounterMap<int> numBurstWriteHits_;

		static CounterMap<int> numIPKReadMisses_;
		static CounterMap<int> numBurstReadMisses_;
		static CounterMap<int> numBurstWriteMisses_;

		static CounterMap<int> numReplaceInvalid_;
		static CounterMap<int> numReplaceClean_;
		static CounterMap<int> numReplaceDirty_;

		// Stats stored from the perspective of each input channel of each core.
		// It would make more sense to use output channels (input channels don't
		// write any data), but we do not have this information at the memory bank.
		struct ChannelStats {
		  count_t readHits;
		  count_t readMisses;
		  count_t writeHits;
		  count_t writeMisses;
		  bool receivingInstructions;
		};
		// Address using coreStats_[core][channel].
		static vector<vector<struct ChannelStats> > coreStats_;

	};

}

#endif /* MEMORYBANK_H_2_ */
