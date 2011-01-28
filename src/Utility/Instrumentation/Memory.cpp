/*
 * Memory.cpp
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#include "Memory.h"

int Memory::instReadCount = 0;
int Memory::dataReadCount = 0;
CounterMap<MemoryAddr> Memory::reads;
CounterMap<MemoryAddr> Memory::writes;

void Memory::read(MemoryAddr address, bool isInstruction) {
  reads.increment(address);
  if(isInstruction) instReadCount++;
  else dataReadCount++;
}

void Memory::write(MemoryAddr address) {
  writes.increment(address);
}

int Memory::numReads()  {return reads.numEvents();}
int Memory::numWrites() {return writes.numEvents();}

void Memory::printStats() {
  int readCount = numReads();
  int writeCount = numWrites();

  if(readCount>0 || writeCount>0) {
    int accesses = readCount + writeCount;

    cout <<
      "Memory:\n" <<
      "  Accesses: " << accesses << "\n" <<
      "    Reads:  " << readCount << "\t(" << asPercentage(readCount,accesses) << ")\n" <<
      "      Instructions: " << instReadCount << "\t(" << asPercentage(instReadCount,readCount) << ")\n" <<
      "      Data:         " << dataReadCount << "\t(" << asPercentage(dataReadCount,readCount) << ")\n" <<
      "    Writes: " << writeCount << "\t(" << asPercentage(writeCount,accesses) << ")" << endl;
  }
}
