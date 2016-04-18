/*
 * MainMemory.cpp
 *
 *  Created on: 18 Apr 2016
 *      Author: db434
 */

#include "MainMemory.h"

#include "../Parameters.h"

using namespace Instrumentation;

count_t MainMemory::numReads_ = 0;
count_t MainMemory::numWrites_ = 0;
count_t MainMemory::numWordsRead_ = 0;
count_t MainMemory::numWordsWritten_ = 0;

void MainMemory::read(MemoryAddr address, uint32_t count) {
  numReads_++;
  numWordsRead_ += count;
}

void MainMemory::write(MemoryAddr address, uint32_t count) {
  numWrites_++;
  numWordsWritten_ += count;
}

count_t MainMemory::numReads()          {return numReads_;}
count_t MainMemory::numWrites()         {return numWrites_;}
count_t MainMemory::numWordsRead()      {return numWordsRead_;}
count_t MainMemory::numWordsWritten()   {return numWordsWritten_;}

void MainMemory::printStats() {
  if (numReads_ > 0 || numWrites_ > 0) {
    count_t accesses = numReads_ + numWrites_;
    count_t words = numWordsRead_ + numWordsWritten_;

    cout <<
      "Main memory:\n" <<
      "    Access requests: " << accesses << "\n" <<
      "  Words transferred: " << words << "\n" <<
      "      Read requests: " << numReads_ << " (" << percentage(numReads_, accesses) << ")\n" <<
      "         Words read: " << numWordsRead_ << " (" << percentage(numWordsRead_, words) << ")\n" <<
      "     Write requests: " << numWrites_ << " (" << percentage(numWrites_, accesses) << ")\n" <<
      "      Words written: " << numWordsWritten_ << " (" << percentage(numWordsWritten_, words) << ")" << endl;
  }
}
