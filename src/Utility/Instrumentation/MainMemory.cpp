/*
 * MainMemory.cpp
 *
 *  Created on: 18 Apr 2016
 *      Author: db434
 */

#include "MainMemory.h"

#include "../Instrumentation.h"
#include "../Parameters.h"

using namespace Instrumentation;

count_t MainMemory::numReads_;
count_t MainMemory::numWrites_;
count_t MainMemory::numWordsRead_;
count_t MainMemory::numWordsWritten_;
count_t MainMemory::numSends_;
count_t MainMemory::numReceives_;

void MainMemory::init() {
  numReads_ = 0;
  numWrites_ = 0;
  numWordsRead_ = 0;
  numWordsWritten_ = 0;
  numSends_ = 0;
  numReceives_ = 0;
}

void MainMemory::read(MemoryAddr address, count_t words) {
  numReads_++;
  numWordsRead_ += words;
}

void MainMemory::write(MemoryAddr address, count_t words) {
  numWrites_++;
  numWordsWritten_ += words;
}

void MainMemory::sendData(NetworkResponse& data) {
  numSends_++;
  // Could also record Hamming distance.
}

void MainMemory::receiveData(NetworkRequest& data) {
  numReceives_++;
  // Could also record Hamming distance.
}

count_t MainMemory::numReads()          {return numReads_;}
count_t MainMemory::numWrites()         {return numWrites_;}
count_t MainMemory::numWordsRead()      {return numWordsRead_;}
count_t MainMemory::numWordsWritten()   {return numWordsWritten_;}

void MainMemory::printStats() {
  if (numReads_ > 0 || numWrites_ > 0) {
    count_t accesses = numReads_ + numWrites_;
    count_t words = numWordsRead_ + numWordsWritten_;

    // Memory can sustain processing one input flit OR one output flit each
    // clock cycle.
    count_t flits = numSends_ + numReceives_;

    cout <<
      "Main memory:\n" <<
      "  Access requests:  " << accesses << "\n" <<
      "    Read requests:  " << numReads_ << " (" << percentage(numReads_, accesses) << ")\n" <<
      "    Words read:     " << numWordsRead_ << " (" << percentage(numWordsRead_, words) << ")\n" <<
      "    Write requests: " << numWrites_ << " (" << percentage(numWrites_, accesses) << ")\n" <<
      "    Words written:  " << numWordsWritten_ << " (" << percentage(numWordsWritten_, words) << ")\n" <<
      "  Bandwidth used:   " << flits << " words (" << percentage(flits, Instrumentation::currentCycle()) << ")" << endl;
  }
}
