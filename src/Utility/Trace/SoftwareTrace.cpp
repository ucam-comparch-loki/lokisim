/*
 * SoftwareTrace.cpp
 *
 *  Created on: 11 Jan 2013
 *      Author: afjk2
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "../Parameters.h"
#include "TraceWriter.h"
#include "SoftwareTrace.h"

static TraceWriter *softwareTraceWriter = NULL;

// Initialize memory tracing engine
void SoftwareTrace::start(const std::string& filename) {
	softwareTraceWriter = new TraceWriter(filename);
}

// Update clock cycle
void SoftwareTrace::setClockCycle(unsigned long long cycleNumber) {
	softwareTraceWriter->setClockCycle(cycleNumber);
}

// Log register file snapshot when requested by syscall
void SoftwareTrace::logRegisterFileSnapshot(unsigned long coreNumber, unsigned long data[], unsigned long count) {
	unsigned long length = count * 4 + 1;
	unsigned char *buffer = new unsigned char[length];
	unsigned long offset = 1;

	buffer[0] = coreNumber;

	for (unsigned long i = 0; i < count; i++) {
		buffer[offset++] = (data[i] >> 24) & 0xFFUL;
		buffer[offset++] = (data[i] >> 16) & 0xFFUL;
		buffer[offset++] = (data[i] >> 8) & 0xFFUL;
		buffer[offset++] = data[i] & 0xFFUL;
	}

	assert(offset == length);

	softwareTraceWriter->writeExtendedRecord(0x01, buffer, length);

	delete[] buffer;
}

// Finalize memory trace
void SoftwareTrace::stop() {
	delete softwareTraceWriter;
}
