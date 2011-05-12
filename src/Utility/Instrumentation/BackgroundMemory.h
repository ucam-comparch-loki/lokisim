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

		static int  numReads();
		static int  numWrites();
		static int  numWordsRead();
		static int  numWordsWritten();

		static void printStats();

	private:

		static int numReads_, numWrites_, numWordsRead_, numWordsWritten_;

	};

}

#endif /* BACKGROUNDMEMORY_H_ */
