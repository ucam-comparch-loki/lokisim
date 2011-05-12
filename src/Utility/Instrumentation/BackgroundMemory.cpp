/*
 * BackgroundMemory.cpp
 *
 *  Created on: 5 May 2011
 *      Author: afjk2
 */

#include "BackgroundMemory.h"

int BackgroundMemory::numReads_ = 0;
int BackgroundMemory::numWrites_ = 0;
int BackgroundMemory::numWordsRead_ = 0;
int BackgroundMemory::numWordsWritten_ = 0;

void BackgroundMemory::read(MemoryAddr address, uint32_t count) {
	numReads_++;
	numWordsRead_ += count;
}

void BackgroundMemory::write(MemoryAddr address, uint32_t count) {
	numWrites_++;
	numWordsWritten_ += count;
}

int BackgroundMemory::numReads()			{return numReads_;}
int BackgroundMemory::numWrites()			{return numWrites_;}
int BackgroundMemory::numWordsRead()		{return numWordsRead_;}
int BackgroundMemory::numWordsWritten()		{return numWordsWritten_;}

void BackgroundMemory::printStats() {
	if (numReads_ > 0 || numWrites_ > 0) {
		int accesses = numReads_ + numWrites_;
		int words = numWordsRead_ + numWordsWritten_;

		cout <<
			"--------------------------------------------------\n" <<
			"On-chip scratchpad\n" <<
			"--------------------------------------------------\n" <<
			"    Access requests: " << accesses << "\n" <<
			"  Words transferred: " << words << "\n" <<
			"      Read requests: " << numReads_ << " (" << percentage(numReads_, accesses) << ")\n" <<
			"         Words read: " << numWordsRead_ << " (" << percentage(numWordsRead_, words) << ")\n" <<
			"     Write requests: " << numWrites_ << " (" << percentage(numWrites_, accesses) << ")\n" <<
			"      Words written: " << numWordsWritten_ << " (" << percentage(numWordsWritten_, words) << ")\n" <<
			"--------------------------------------------------" << endl;
	}
}
