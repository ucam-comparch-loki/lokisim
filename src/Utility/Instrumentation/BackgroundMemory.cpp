/*
 * BackgroundMemory.cpp
 *
 *  Created on: 5 May 2011
 *      Author: afjk2
 */

#include "../Parameters.h"
#include "BackgroundMemory.h"

unsigned long long BackgroundMemory::numReads_ = 0;
unsigned long long BackgroundMemory::numWrites_ = 0;
unsigned long long BackgroundMemory::numWordsRead_ = 0;
unsigned long long BackgroundMemory::numWordsWritten_ = 0;

void BackgroundMemory::read(MemoryAddr address, uint32_t count) {
	numReads_++;
	numWordsRead_ += count;
}

void BackgroundMemory::write(MemoryAddr address, uint32_t count) {
	numWrites_++;
	numWordsWritten_ += count;
}

unsigned long long BackgroundMemory::numReads()			{return numReads_;}
unsigned long long BackgroundMemory::numWrites()			{return numWrites_;}
unsigned long long BackgroundMemory::numWordsRead()		{return numWordsRead_;}
unsigned long long BackgroundMemory::numWordsWritten()		{return numWordsWritten_;}

void BackgroundMemory::printStats() {
	if (BATCH_MODE) {
		cout << "<@GLOBAL>bgmem_read_reqs:" << numReads_ << "</@GLOBAL>" << endl;
		cout << "<@GLOBAL>bgmem_read_words:" << numWordsRead_ << "</@GLOBAL>" << endl;
		cout << "<@GLOBAL>bgmem_write_reqs:" << numWrites_ << "</@GLOBAL>" << endl;
		cout << "<@GLOBAL>bgmem_write_words:" << numWordsWritten_ << "</@GLOBAL>" << endl;
	}

	if (numReads_ > 0 || numWrites_ > 0) {
		unsigned long long accesses = numReads_ + numWrites_;
		unsigned long long words = numWordsRead_ + numWordsWritten_;

		cout <<
			"On-chip scratchpad:\n" <<
			"    Access requests: " << accesses << "\n" <<
			"  Words transferred: " << words << "\n" <<
			"      Read requests: " << numReads_ << " (" << percentage(numReads_, accesses) << ")\n" <<
			"         Words read: " << numWordsRead_ << " (" << percentage(numWordsRead_, words) << ")\n" <<
			"     Write requests: " << numWrites_ << " (" << percentage(numWrites_, accesses) << ")\n" <<
			"      Words written: " << numWordsWritten_ << " (" << percentage(numWordsWritten_, words) << ")" << endl;
	}
}
