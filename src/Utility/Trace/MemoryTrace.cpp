/*
 * MemoryTrace.cpp
 *
 *  Created on: 13 Sep 2011
 *      Author: afjk2
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "../Parameters.h"
#include "TraceWriter.h"
#include "MemoryTrace.h"

static TraceWriter *memoryTraceWriter = NULL;

// Initialize memory tracing engine
void MemoryTrace::start(const std::string& filename) {
	memoryTraceWriter = new TraceWriter(filename);
}

// Update clock cycle
void MemoryTrace::setClockCycle(unsigned long long cycleNumber) {
	memoryTraceWriter->setClockCycle(cycleNumber);
}

// Log memory accesses
void MemoryTrace::readIPKWord(unsigned long bankNumber, unsigned long address) {
	memoryTraceWriter->writeRecord(0x01, bankNumber, address);
}

void MemoryTrace::readWord(unsigned long bankNumber, unsigned long address) {
	memoryTraceWriter->writeRecord(0x02, bankNumber, address);
}

void MemoryTrace::readHalfWord(unsigned long bankNumber, unsigned long address) {
	memoryTraceWriter->writeRecord(0x03, bankNumber, address);
}

void MemoryTrace::readByte(unsigned long bankNumber, unsigned long address) {
	memoryTraceWriter->writeRecord(0x04, bankNumber, address);
}

void MemoryTrace::writeWord(unsigned long bankNumber, unsigned long address) {
	memoryTraceWriter->writeRecord(0x05, bankNumber, address);
}

void MemoryTrace::writeHalfWord(unsigned long bankNumber, unsigned long address) {
	memoryTraceWriter->writeRecord(0x06, bankNumber, address);
}

void MemoryTrace::writeByte(unsigned long bankNumber, unsigned long address) {
	memoryTraceWriter->writeRecord(0x07, bankNumber, address);
}

void MemoryTrace::splitLineIPK(unsigned long bankNumber, unsigned long address) {
	memoryTraceWriter->writeRecord(0x08, bankNumber, address);
}

void MemoryTrace::splitBankIPK(unsigned long bankNumber, unsigned long address) {
	memoryTraceWriter->writeRecord(0x09, bankNumber, address);
}

void MemoryTrace::stopIPK(unsigned long bankNumber, unsigned long address) {
	memoryTraceWriter->writeRecord(0x0A, bankNumber, address);
}

// Finalize memory trace
void MemoryTrace::stop() {
	delete memoryTraceWriter;
}
