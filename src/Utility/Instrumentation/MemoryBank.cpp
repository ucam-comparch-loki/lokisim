/*
 * MemoryBank.cpp
 *
 *  Created on: 5 May 2011
 *      Author: afjk2
 */

#include "MemoryBank.h"

#include <map>

std::map<int, bool> MemoryBank::modes_;

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

void MemoryBank::setMode(int bank, bool isCache) {
	modes_[bank] = isCache;
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
		cout <<
			"--------------------------------------------------\n" <<
			"Memory banks\n" <<
			"--------------------------------------------------" << endl;

		std::map<int, bool>::iterator it;

		for (it = modes_.begin(); it != modes_.end(); it++) {
			int bank = it->first;
			bool isCache = it->second;

			cout << "                        Bank: " << bank << endl;
			cout << "                        Mode: " << (isCache ? "General purpose cache" : "Scratchpad") << endl;

			int scalarReadCount = 0;
			int scalarWriteCount = 0;
			int hits;
			int misses;

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

			int starts;
			int conts;

			starts = numStartIPKRead_[bank];	conts = numContinueIPKRead_[bank];
			cout << "              IPK line reads: " << starts << " / " << conts << " (" << (starts + conts) << ")" << endl;
			starts = numStartBurstRead_[bank];	conts = numContinueBurstRead_[bank];
			cout << "            Burst line reads: " << starts << " / " << conts << " (" << (starts + conts) << ")" << endl;
			starts = numStartBurstWrite_[bank];	conts = numContinueBurstWrite_[bank];
			cout << "           Burst line writes: " << starts << " / " << conts << " (" << (starts + conts) << ")" << endl;

			int burstReadCount = 0;
			int burstWriteCount = 0;

			hits = numIPKReadHits_[bank];	misses = numIPKReadMisses_[bank];	burstReadCount += hits + misses;
			cout << "              IPK word reads: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;
			hits = numBurstReadHits_[bank];	misses = numBurstReadMisses_[bank];	burstReadCount += hits + misses;
			cout << "            Burst word reads: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;
			hits = numBurstWriteHits_[bank];	misses = numBurstWriteMisses_[bank];	burstWriteCount += hits + misses;
			cout << "           Burst word writes: " << hits << " / " << misses << " (" << percentage(hits, hits + misses) << ")" << endl;

			cout << "            Burst word reads: " << burstReadCount << " (" << percentage(burstReadCount, burstReadCount + burstWriteCount) << ")" << endl;
			cout << "           Burst word writes: " << burstWriteCount << " (" << percentage(burstWriteCount, burstReadCount + burstWriteCount) << ")" << endl;

			int cacheInvalid = numReplaceInvalid_[bank];
			int cacheClean = numReplaceClean_[bank];
			int cacheDirty = numReplaceDirty_[bank];
			int cacheTotal = cacheInvalid + cacheClean + cacheDirty;

			cout << "      Replaced invalid lines: " << cacheInvalid << " (" << percentage(cacheInvalid, cacheTotal) << ")" << endl;
			cout << "        Replaced clean lines: " << cacheClean << " (" << percentage(cacheClean, cacheTotal) << ")" << endl;
			cout << "        Replaced dirty lines: " << cacheDirty << " (" << percentage(cacheDirty, cacheTotal) << ")" << endl;

			int ringHandOffs = numHandOffRequests_[bank];
			int ringPassThroughs = numPassThroughRequests_[bank];

			cout << "      Ring hand-off messages: " << ringHandOffs << " (" << percentage(ringHandOffs, ringHandOffs + ringPassThroughs) << ")" << endl;
			cout << "  Ring pass-through messages: " << ringPassThroughs << " (" << percentage(ringPassThroughs, ringHandOffs + ringPassThroughs) << ")" << endl;

			cout << "--------------------------------------------------" << endl;
		}
	}
}
