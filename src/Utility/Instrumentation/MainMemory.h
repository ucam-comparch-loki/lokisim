/*
 * MainMemory.h
 *
 *  Created on: 18 Apr 2016
 *      Author: db434
 */

#ifndef SRC_UTILITY_INSTRUMENTATION_MAINMEMORY_H_
#define SRC_UTILITY_INSTRUMENTATION_MAINMEMORY_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"

namespace Instrumentation {

  class MainMemory : public InstrumentationBase {

  public:

    static void reset();

    static void read(MemoryAddr address, count_t words);
    static void write(MemoryAddr address, count_t words);

    static void sendData(NetworkResponse& data);
    static void receiveData(NetworkRequest& data);

    static count_t numReads();
    static count_t numWrites();
    static count_t numWordsRead();
    static count_t numWordsWritten();

    static void printStats(const chip_parameters_t& params);

  private:

    static count_t numReads_, numWrites_, numWordsRead_, numWordsWritten_;
    static count_t numSends_, numReceives_;

  };

}

#endif /* SRC_UTILITY_INSTRUMENTATION_MAINMEMORY_H_ */
