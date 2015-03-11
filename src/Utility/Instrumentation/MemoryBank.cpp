/*
 * MemoryBank.cpp
 *
 *  Created on: 5 May 2011
 *      Author: afjk2
 */

#include "../Parameters.h"
#include "MemoryBank.h"

#include <map>

#include "IPKCache.h"

using namespace Instrumentation;

std::map<int, bool> MemoryBank::modes_;
std::map<int, uint> MemoryBank::setCounts_;
std::map<int, uint> MemoryBank::wayCounts_;
std::map<int, uint> MemoryBank::lineSizes_;

CounterMap<int> MemoryBank::numReadWordHits_;
CounterMap<int> MemoryBank::numReadHalfWordHits_;
CounterMap<int> MemoryBank::numReadByteHits_;

CounterMap<int> MemoryBank::numWriteWordHits_;
CounterMap<int> MemoryBank::numWriteHalfWordHits_;
CounterMap<int> MemoryBank::numWriteByteHits_;

CounterMap<int> MemoryBank::numReadWordMisses_;
CounterMap<int> MemoryBank::numReadHalfWordMisses_;
CounterMap<int> MemoryBank::numReadByteMisses_;

CounterMap<int> MemoryBank::numWriteWordMisses_;
CounterMap<int> MemoryBank::numWriteHalfWordMisses_;
CounterMap<int> MemoryBank::numWriteByteMisses_;

CounterMap<int> MemoryBank::numStartIPKRead_;
CounterMap<int> MemoryBank::numStartBurstRead_;
CounterMap<int> MemoryBank::numStartBurstWrite_;

CounterMap<int> MemoryBank::numContinueIPKRead_;
CounterMap<int> MemoryBank::numContinueBurstRead_;
CounterMap<int> MemoryBank::numContinueBurstWrite_;

CounterMap<int> MemoryBank::numIPKReadHits_;
CounterMap<int> MemoryBank::numBurstReadHits_;
CounterMap<int> MemoryBank::numBurstWriteHits_;

CounterMap<int> MemoryBank::numIPKReadMisses_;
CounterMap<int> MemoryBank::numBurstReadMisses_;
CounterMap<int> MemoryBank::numBurstWriteMisses_;

CounterMap<int> MemoryBank::numReplaceInvalid_;
CounterMap<int> MemoryBank::numReplaceClean_;
CounterMap<int> MemoryBank::numReplaceDirty_;

CounterMap<int> MemoryBank::numHandOffRequests_;
CounterMap<int> MemoryBank::numPassThroughRequests_;

void MemoryBank::setMode(int bank, bool isCache, uint setCount, uint wayCount, uint lineSize) {
	modes_[bank] = isCache;
	setCounts_[bank] = setCount;
	wayCounts_[bank] = wayCount;
	lineSizes_[bank] = lineSize;
}

void MemoryBank::readWord(int bank, MemoryAddr address, bool isMiss) {
	if (isMiss)
		numReadWordMisses_.increment(bank);
	else
		numReadWordHits_.increment(bank);
}

void MemoryBank::readHalfWord(int bank, MemoryAddr address, bool isMiss) {
	if (isMiss)
		numReadHalfWordMisses_.increment(bank);
	else
		numReadHalfWordHits_.increment(bank);
}

void MemoryBank::readByte(int bank, MemoryAddr address, bool isMiss) {
	if (isMiss)
		numReadByteMisses_.increment(bank);
	else
		numReadByteHits_.increment(bank);
}

void MemoryBank::writeWord(int bank, MemoryAddr address, bool isMiss) {
	if (isMiss)
		numWriteWordMisses_.increment(bank);
	else
		numWriteWordHits_.increment(bank);
}

void MemoryBank::writeHalfWord(int bank, MemoryAddr address, bool isMiss) {
	if (isMiss)
		numWriteHalfWordMisses_.increment(bank);
	else
		numWriteHalfWordHits_.increment(bank);
}

void MemoryBank::writeByte(int bank, MemoryAddr address, bool isMiss) {
	if (isMiss)
		numWriteByteMisses_.increment(bank);
	else
		numWriteByteHits_.increment(bank);
}

void MemoryBank::initiateIPKRead(int bank, bool isHandOff) {
	if (isHandOff)
		numContinueIPKRead_.increment(bank);
	else
		numStartIPKRead_.increment(bank);
}

void MemoryBank::initiateBurstRead(int bank, bool isHandOff) {
	if (isHandOff)
		numContinueBurstRead_.increment(bank);
	else
		numStartBurstRead_.increment(bank);
}

void MemoryBank::initiateBurstWrite(int bank, bool isHandOff) {
	if (isHandOff)
		numContinueBurstWrite_.increment(bank);
	else
		numStartBurstWrite_.increment(bank);
}

void MemoryBank::readIPKWord(int bank, MemoryAddr address, bool isMiss) {
	if (isMiss)
		numIPKReadMisses_.increment(bank);
	else
		numIPKReadHits_.increment(bank);
}

void MemoryBank::readBurstWord(int bank, MemoryAddr address, bool isMiss) {
	if (isMiss)
		numBurstReadMisses_.increment(bank);
	else
		numBurstReadHits_.increment(bank);
}

void MemoryBank::writeBurstWord(int bank, MemoryAddr address, bool isMiss) {
	if (isMiss)
		numBurstWriteMisses_.increment(bank);
	else
		numBurstWriteHits_.increment(bank);
}

void MemoryBank::replaceCacheLine(int bank, bool isValid, bool isDirty) {
	if (!isValid)
		numReplaceInvalid_.increment(bank);
	else if (isDirty)
		numReplaceDirty_.increment(bank);
	else
		numReplaceClean_.increment(bank);
}

void MemoryBank::ringHandOff(int bank) {
	numHandOffRequests_.increment(bank);
}

void MemoryBank::ringPassThrough(int bank) {
	numPassThroughRequests_.increment(bank);
}

void MemoryBank::printStats() {
	if (!modes_.empty()) {
        if (BATCH_MODE) {
    		std::map<int, bool>::iterator it;

    		for (it = modes_.begin(); it != modes_.end(); it++) {
    			int bank = it->first;
    			bool isCache = it->second;

            	cout << "<@SUBTABLE>memory_banks";

    			cout << "!bank_number:" << bank;
    			cout << "!bank_mode:" << (isCache ? "General purpose cache" : "Scratchpad");
    			cout << "!set_count:" << setCounts_[bank];
    			cout << "!way_count:" << wayCounts_[bank];
    			cout << "!line_size:" << lineSizes_[bank];

    			cout << "!load_w_hits:" << numReadWordHits_[bank];
    			cout << "!load_w_misses:" << numReadWordMisses_[bank];
    			cout << "!load_hw_hits:" << numReadHalfWordHits_[bank];
    			cout << "!load_hw_misses:" << numReadHalfWordMisses_[bank];
    			cout << "!load_b_hits:" << numReadByteHits_[bank];
    			cout << "!load_b_misses:" << numReadByteMisses_[bank];

    			cout << "!store_w_hits:" << numWriteWordHits_[bank];
    			cout << "!store_w_misses:" << numWriteWordMisses_[bank];
    			cout << "!store_hw_hits:" << numWriteHalfWordHits_[bank];
    			cout << "!store_hw_misses:" << numWriteHalfWordMisses_[bank];
    			cout << "!store_b_hits:" << numWriteByteHits_[bank];
    			cout << "!store_b_misses:" << numWriteByteMisses_[bank];

    			cout << "!ipkread_l_starts:" << numStartIPKRead_[bank];
    			cout << "!ipkread_l_conts:" << numContinueIPKRead_[bank];
    			cout << "!burstread_l_starts:" << numStartBurstRead_[bank];
    			cout << "!burstread_l_conts:" << numContinueBurstRead_[bank];
    			cout << "!burstwrite_l_starts:" << numStartBurstWrite_[bank];
    			cout << "!burstwrite_l_conts:" << numContinueBurstWrite_[bank];

    			cout << "!ipkread_w_hits:" << numIPKReadHits_[bank];
    			cout << "!ipkread_w_misses:" << numIPKReadMisses_[bank];
    			cout << "!burstread_w_hits:" << numBurstReadHits_[bank];
    			cout << "!burstread_w_misses:" << numBurstReadMisses_[bank];
    			cout << "!burstwrite_w_hits:" << numBurstWriteHits_[bank];
    			cout << "!burstwrite_w_misses:" << numBurstWriteMisses_[bank];

    			cout << "!replace_invalid:" << numReplaceInvalid_[bank];
    			cout << "!replace_clean:" << numReplaceClean_[bank];
    			cout << "!replace_dirty:" << numReplaceDirty_[bank];

    			cout << "!ring_hand_offs:" << numHandOffRequests_[bank];
    			cout << "!ring_pass_through:" << numPassThroughRequests_[bank];

            	cout << "</@SUBTABLE>" << endl;
    		}
        }

		cout << "Memory banks:" << endl;

		std::map<int, bool>::iterator it;
		bool bankOutput = false;

		for (it = modes_.begin(); it != modes_.end(); it++) {
			int bank = it->first;
			bool isCache = it->second;

			if (bankOutput)
				cout << "  --------------------------------------------------" << endl;

			bankOutput = true;

			cout << "                        Bank: " << bank << endl;
			cout << "                        Mode: " << (isCache ? "General purpose cache" : "Scratchpad") << endl;
			cout << "                   Set count: " << setCounts_[bank] << endl;
			cout << "                   Way count: " << wayCounts_[bank] << endl;
			cout << "                   Line size: " << lineSizes_[bank] << endl;

			unsigned long long scalarReadCount = 0;
			unsigned long long scalarWriteCount = 0;
			unsigned long long hits;
			unsigned long long misses;

			hits = numReadWordHits_[bank];	misses = numReadWordMisses_[bank];	scalarReadCount += hits + misses;
			cout << "           Scalar word reads: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;
			hits = numReadHalfWordHits_[bank];	misses = numReadHalfWordMisses_[bank];	scalarReadCount += hits + misses;
			cout << "      Scalar half-word reads: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;
			hits = numReadByteHits_[bank];	misses = numReadByteMisses_[bank];	scalarReadCount += hits + misses;
			cout << "           Scalar byte reads: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;

			hits = numWriteWordHits_[bank];	misses = numWriteWordMisses_[bank];	scalarWriteCount += hits + misses;
			cout << "          Scalar word writes: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;
			hits = numWriteHalfWordHits_[bank];	misses = numWriteHalfWordMisses_[bank];	scalarWriteCount += hits + misses;
			cout << "     Scalar half-word writes: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;
			hits = numWriteByteHits_[bank];	misses = numWriteByteMisses_[bank];	scalarWriteCount += hits + misses;
			cout << "          Scalar byte writes: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;

			cout << "        Scalar read requests: " << scalarReadCount << " (" << percentage(scalarReadCount, scalarReadCount + scalarWriteCount) << ")" << endl;
			cout << "       Scalar write requests: " << scalarWriteCount << " (" << percentage(scalarWriteCount, scalarReadCount + scalarWriteCount) << ")" << endl;

			unsigned long long starts;
			unsigned long long conts;

			starts = numStartIPKRead_[bank];	conts = numContinueIPKRead_[bank];
			cout << "              IPK line reads: " << starts << " / " << conts << " (" << (starts + conts) << ")" << endl;
			starts = numStartBurstRead_[bank];	conts = numContinueBurstRead_[bank];
			cout << "            Burst line reads: " << starts << " / " << conts << " (" << (starts + conts) << ")" << endl;
			starts = numStartBurstWrite_[bank];	conts = numContinueBurstWrite_[bank];
			cout << "           Burst line writes: " << starts << " / " << conts << " (" << (starts + conts) << ")" << endl;

			unsigned long long burstReadCount = 0;
			unsigned long long burstWriteCount = 0;

			hits = numIPKReadHits_[bank];	misses = numIPKReadMisses_[bank];	burstReadCount += hits + misses;
			cout << "              IPK word reads: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;
			hits = numBurstReadHits_[bank];	misses = numBurstReadMisses_[bank];	burstReadCount += hits + misses;
			cout << "            Burst word reads: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;
			hits = numBurstWriteHits_[bank];	misses = numBurstWriteMisses_[bank];	burstWriteCount += hits + misses;
			cout << "           Burst word writes: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;

			cout << "        Streaming word reads: " << burstReadCount << " (" << percentage(burstReadCount, burstReadCount + burstWriteCount) << ")" << endl;
			cout << "       Streaming word writes: " << burstWriteCount << " (" << percentage(burstWriteCount, burstReadCount + burstWriteCount) << ")" << endl;

			unsigned long long cacheInvalid = numReplaceInvalid_[bank];
			unsigned long long cacheClean = numReplaceClean_[bank];
			unsigned long long cacheDirty = numReplaceDirty_[bank];
			unsigned long long cacheTotal = cacheInvalid + cacheClean + cacheDirty;

			cout << "      Replaced invalid lines: " << cacheInvalid << " (" << percentage(cacheInvalid, cacheTotal) << ")" << endl;
			cout << "        Replaced clean lines: " << cacheClean << " (" << percentage(cacheClean, cacheTotal) << ")" << endl;
			cout << "        Replaced dirty lines: " << cacheDirty << " (" << percentage(cacheDirty, cacheTotal) << ")" << endl;

			unsigned long long ringHandOffs = numHandOffRequests_[bank];
			unsigned long long ringPassThroughs = numPassThroughRequests_[bank];

			cout << "      Ring hand-off messages: " << ringHandOffs << " (" << percentage(ringHandOffs, ringHandOffs + ringPassThroughs) << ")" << endl;
			cout << "  Ring pass-through messages: " << ringPassThroughs << " (" << percentage(ringPassThroughs, ringHandOffs + ringPassThroughs) << ")" << endl;
		}
	}
}

void MemoryBank::printSummary() {
  count_t instructionReadHits = numIPKReadHits_.numEvents();
  count_t instructionReads = instructionReadHits + numIPKReadMisses_.numEvents();
  count_t l0l1InstReads = IPKCache::numReads();
  count_t l0l1InstReadHits = l0l1InstReads - numIPKReadMisses_.numEvents();
  count_t dataReadHits = numReadByteHits_.numEvents() + numReadHalfWordHits_.numEvents() + numReadWordHits_.numEvents();
  count_t dataReads = dataReadHits + numReadByteMisses_.numEvents() + numReadHalfWordMisses_.numEvents() + numReadWordMisses_.numEvents();
  count_t dataWriteHits = numWriteByteHits_.numEvents() + numWriteHalfWordHits_.numEvents() + numWriteWordHits_.numEvents();
  count_t dataWrites = dataWriteHits + numWriteByteMisses_.numEvents() + numWriteHalfWordMisses_.numEvents() + numWriteWordMisses_.numEvents();
  count_t totalHits = instructionReadHits + dataReadHits + dataWriteHits;
  count_t totalAccesses = instructionReads + dataReads + dataWrites;

  using std::clog;

  // Note that for every miss event, there will be a corresponding hit event
  // when the data arrives. We may prefer to filter out these "duplicates".
  clog << "L1 cache activity:" << endl;
  clog << "  Instruction hits: " << instructionReadHits << "/" << instructionReads << " (" << percentage(instructionReadHits, instructionReads) << ")\n";
  clog << "  L0+L1 inst hits:  " << l0l1InstReadHits << "/" << l0l1InstReads << " (" << percentage(l0l1InstReadHits,l0l1InstReads) << ")" << endl;
  clog << "  Data read hits:   " << dataReadHits << "/" << dataReads << " (" << percentage(dataReadHits, dataReads) << ")\n";
  clog << "  Data write hits:  " << dataWriteHits << "/" << dataWrites << " (" << percentage(dataWriteHits, dataWrites) << ")\n";
  clog << "  Total hits:       " << totalHits << "/" << totalAccesses << " (" << percentage(totalHits, totalAccesses) << ")\n";
}

void MemoryBank::dumpEventCounts(std::ostream& os) {
  os << "<memory size=\"" << MEMORY_BANK_SIZE << "\">\n"
     << xmlNode("instances", NUM_MEMORIES)                                << "\n"
     << xmlNode("active", numReads() + numWrites())  /* is this right? */ << "\n"
     << xmlNode("read", numReads())                                       << "\n"
     << xmlNode("write", numWrites())                                     << "\n"
     << xmlNode("word_read", numReadWordHits_.numEvents())                << "\n"
     << xmlNode("halfword_read", numReadHalfWordHits_.numEvents())        << "\n"
     << xmlNode("byte_read", numReadByteHits_.numEvents())                << "\n"
     << xmlNode("ipk_read", numIPKReadHits_.numEvents())                  << "\n"
     << xmlNode("burst_read", numBurstReadHits_.numEvents())              << "\n"
     << xmlNode("word_write", numWriteWordHits_.numEvents())              << "\n"
     << xmlNode("halfword_write", numWriteHalfWordHits_.numEvents())      << "\n"
     << xmlNode("byte_write", numWriteByteHits_.numEvents())              << "\n"
     << xmlNode("burst_write", numBurstWriteHits_.numEvents())            << "\n"
     << xmlNode("replace_line", numReplaceClean_.numEvents() + numReplaceDirty_.numEvents() + numReplaceInvalid_.numEvents()) << "\n"
     << xmlNode("ring_hand_off", numHandOffRequests_.numEvents())         << "\n"
     << xmlNode("ring_pass_through", numPassThroughRequests_.numEvents()) << "\n"
     << xmlEnd("memory")                                                  << "\n";
}

long long MemoryBank::numReads() {
  return numReadWordHits_.numEvents()
       + numReadHalfWordHits_.numEvents()
       + numReadByteHits_.numEvents()
       + numIPKReadHits_.numEvents()
       + numBurstReadHits_.numEvents();
}

long long MemoryBank::numWrites() {
  return numWriteWordHits_.numEvents()
       + numWriteHalfWordHits_.numEvents()
       + numWriteByteHits_.numEvents()
       + numBurstWriteHits_.numEvents();
}

long long MemoryBank::numIPKReadMisses() {
  return numIPKReadMisses_.numEvents();
}
