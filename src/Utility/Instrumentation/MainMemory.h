/*
 * MainMemory.h
 *
 *  Created on: 18 Apr 2016
 *      Author: db434
 */

#ifndef MAINMEMORY_H_
#define MAINMEMORY_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"

namespace Instrumentation {

  class MainMemory : public InstrumentationBase {

  public:

    static void read(MemoryAddr address, uint32_t count);
    static void write(MemoryAddr address, uint32_t count);

    static count_t numReads();
    static count_t numWrites();
    static count_t numWordsRead();
    static count_t numWordsWritten();

    static void printStats();

  private:

    static count_t numReads_, numWrites_, numWordsRead_, numWordsWritten_;

  };

}

#endif /* MAINMEMORY_H_ */
