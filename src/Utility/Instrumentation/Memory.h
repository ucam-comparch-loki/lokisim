/*
 * Memory.h
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"

namespace Instrumentation {

class Memory : public InstrumentationBase {

public:

  static void read(MemoryAddr address, bool isInstruction);
  static void write(MemoryAddr address);
  static void tagCheck(MemoryAddr address, bool hit);

  static int  numReads();
  static int  numWrites();
  static int  numHits();
  static int  numMisses();
  static int  numTagChecks();

  static void printStats();

private:

  static CounterMap<MemoryAddr> reads, writes;
  static int instReadCount_, dataReadCount_, numHits_, numMisses_;

};

}

#endif /* MEMORY_H_ */
