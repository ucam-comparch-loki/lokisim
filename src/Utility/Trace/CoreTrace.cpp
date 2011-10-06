/*
 * CoreTrace.cpp
 *
 *  Created on: 28 Sep 2011
 *      Author: afjk2
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include <zlib.h>

#include "../Parameters.h"
#include "TraceWriter.h"
#include "CoreTrace.h"

static TraceWriter *coreTraceWriter = NULL;

// Initialize memory tracing engine
void CoreTrace::start(const std::string& filename) {
	coreTraceWriter = new TraceWriter(filename);
}

// Update clock cycle
void CoreTrace::setClockCycle(unsigned long long cycleNumber) {
	coreTraceWriter->setClockCycle(cycleNumber);
}

// Log core activity
void CoreTrace::decodeInstruction(unsigned long coreNumber, unsigned long address, bool endOfPacket) {
	coreTraceWriter->writeRecord(endOfPacket ? 0x02 : 0x01, coreNumber, address);
}

// Finalize memory trace
void CoreTrace::stop() {
	delete coreTraceWriter;
}
