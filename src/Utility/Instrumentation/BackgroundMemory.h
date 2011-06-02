/*
 * BackgroundMemory.h
 *
 *  Created on: 5 May 2011
 *      Author: afjk2
 */

#ifndef BACKGROUNDMEMORY_H_
#define BACKGROUNDMEMORY_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"

namespace Instrumentation {

	class BackgroundMemory : public InstrumentationBase {

	public:

		static void read(MemoryAddr address, uint32_t count);
		static void write(MemoryAddr address, uint32_t count);

		static unsigned long long  numReads();
		static unsigned long long  numWrites();
		static unsigned long long  numWordsRead();
		static unsigned long long  numWordsWritten();

		static void printStats();

	private:

		static unsigned long long numReads_, numWrites_, numWordsRead_, numWordsWritten_;

	};

}

#endif /* BACKGROUNDMEMORY_H_ */
