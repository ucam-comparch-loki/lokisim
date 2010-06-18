/*
 * Memory.cpp
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#include "Memory.h"

int Memory::readCount = 0;
int Memory::writeCount = 0;

void Memory::read() {
  readCount++;
}

void Memory::write() {
  writeCount++;
}

void Memory::printStats() {
  if(readCount>0 || writeCount>0) {
    int accesses = readCount + writeCount;

    cout <<
      "Memory:" << endl <<
      "  Accesses: " << accesses << endl <<
      "    Reads:  " << readCount << "\t(" << asPercentage(readCount,accesses) << ")" << endl <<
      "    Writes: " << writeCount << "\t(" << asPercentage(writeCount,accesses) << ")" << endl;
  }
}
