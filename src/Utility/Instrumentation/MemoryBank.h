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

namespace Instrumentation {

	class MemoryBank : public InstrumentationBase {

	public:

		static void setMode(int bank, bool isCache, uint setCount, uint wayCount, uint lineSize);

		static void readWord(int bank, MemoryAddr address, bool isMiss);
		static void readHalfWord(int bank, MemoryAddr address, bool isMiss);
		static void readByte(int bank, MemoryAddr address, bool isMiss);

		static void writeWord(int bank, MemoryAddr address, bool isMiss);
		static void writeHalfWord(int bank, MemoryAddr address, bool isMiss);
		static void writeByte(int bank, MemoryAddr address, bool isMiss);

		static void initiateIPKRead(int bank, bool isHandOff);
		static void initiateBurstRead(int bank, bool isHandOff);
		static void initiateBurstWrite(int bank, bool isHandOff);

		static void readIPKWord(int bank, MemoryAddr address, bool isMiss);
		static void readBurstWord(int bank, MemoryAddr address, bool isMiss);
		static void writeBurstWord(int bank, MemoryAddr address, bool isMiss);

		static void replaceCacheLine(int bank, bool isValid, bool isDirty);

		static void ringHandOff(int bank);
		static void ringPassThrough(int bank);

		static void printStats();

	private:

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

		static CounterMap<int> numHandOffRequests_;
		static CounterMap<int> numPassThroughRequests_;

	};

}

#endif /* MEMORYBANK_H_2_ */
