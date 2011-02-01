/*
 * Memory.cpp
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#include "Memory.h"

int Memory::instReadCount_ = 0;
int Memory::dataReadCount_ = 0;
int Memory::numHits_ = 0;
int Memory::numMisses_ = 0;
CounterMap<MemoryAddr> Memory::reads;
CounterMap<MemoryAddr> Memory::writes;

void Memory::read(MemoryAddr address, bool isInstruction) {
  reads.increment(address);
  if(isInstruction) instReadCount_++;
  else dataReadCount_++;
}

void Memory::write(MemoryAddr address) {
  writes.increment(address);
}

void Memory::tagCheck(MemoryAddr address, bool hit) {
  if(hit) numHits_++;
  else    numMisses_++;
}

int Memory::numReads()     {return reads.numEvents();}
int Memory::numWrites()    {return writes.numEvents();}
int Memory::numHits()      {return numHits_;}
int Memory::numMisses()    {return numMisses_;}
int Memory::numTagChecks() {return numHits() + numMisses();}

void Memory::printStats() {
  int readCount = numReads();
  int writeCount = numWrites();

  if(readCount>0 || writeCount>0) {
    int accesses = readCount + writeCount;

    cout <<
      "Memory:\n" <<
      "  Accesses: " << accesses << "\n" <<
      "    Reads:  " << readCount << "\t(" << percentage(readCount,accesses) << ")\n" <<
      "      Instructions: " << instReadCount_ << "\t(" << percentage(instReadCount_,readCount) << ")\n" <<
      "      Data:         " << dataReadCount_ << "\t(" << percentage(dataReadCount_,readCount) << ")\n" <<
      "    Writes: " << writeCount << "\t(" << percentage(writeCount,accesses) << ")" << endl;
  }
}
