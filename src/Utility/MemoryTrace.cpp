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

#include <zlib.h>

#include "Parameters.h"
#include "MemoryTrace.h"

#define MEMORY_TRACE_BUFFER_SIZE (64 * 1024)
#define MEMORY_TRACE_BUFFER_THRESHOLD (MEMORY_TRACE_BUFFER_SIZE - 32)

static FILE *traceFile = NULL;

static unsigned char *traceBuffer = NULL;
static unsigned char *traceCompressBuffer = NULL;
static size_t traceBufferCursor = 0;

static unsigned long long currentCycleNumber = 0;

z_stream gzipStream;

static void flushTraceBuffer(bool finish) {
	if (traceBufferCursor == 0 && !finish)
		return;

	gzipStream.next_in = (Bytef*)traceBuffer;
	gzipStream.avail_in = (uInt)traceBufferCursor;
	gzipStream.next_out = traceCompressBuffer;
	gzipStream.avail_out = (uInt)MEMORY_TRACE_BUFFER_SIZE;

    int err = deflate(&gzipStream, finish ? Z_FINISH : Z_NO_FLUSH);
    assert((finish && err == Z_STREAM_END) || (!finish && err == Z_OK));

    if (gzipStream.avail_in > 0)
    	memmove(traceBuffer, traceBuffer + traceBufferCursor - gzipStream.avail_in, gzipStream.avail_in);

    traceBufferCursor = gzipStream.avail_in;
    assert((finish && traceBufferCursor == 0) || (!finish && traceBufferCursor < MEMORY_TRACE_BUFFER_THRESHOLD));

    size_t bytesOut = gzipStream.next_out - traceCompressBuffer;
	size_t count = fwrite(traceCompressBuffer, 1, bytesOut, traceFile);
	assert(count == bytesOut);
}

// Initialize memory tracing engine
void MemoryTrace::start(const std::string& filename) {
	traceFile = fopen(filename.c_str(), "wb");

	traceBuffer = new unsigned char[MEMORY_TRACE_BUFFER_SIZE];
	traceCompressBuffer = new unsigned char[MEMORY_TRACE_BUFFER_SIZE];
	traceBufferCursor = 0;

	gzipStream.next_in = (Bytef*)traceBuffer;
    gzipStream.avail_in = (uInt)0;
    gzipStream.next_out = traceCompressBuffer;
    gzipStream.avail_out = (uInt)MEMORY_TRACE_BUFFER_SIZE;

    gzipStream.zalloc = (alloc_func)0;
    gzipStream.zfree = (free_func)0;
    gzipStream.opaque = (voidpf)0;

    int err = deflateInit2(&gzipStream, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 9, Z_DEFAULT_STRATEGY);
    assert(err == Z_OK);
}

// Update clock cycle
void MemoryTrace::setClockCycle(unsigned long long cycleNumber) {
	currentCycleNumber = cycleNumber;
}

// Log memory accesses
void MemoryTrace::readIPKWord(unsigned long bankNumber, unsigned long address) {
	traceBuffer[traceBufferCursor++] = 0x01;
	traceBuffer[traceBufferCursor++] = (unsigned char)bankNumber;
	*((unsigned long*)(traceBuffer + traceBufferCursor)) = address;
	traceBufferCursor += 4;
	*((unsigned long long*)(traceBuffer + traceBufferCursor)) = currentCycleNumber;
	traceBufferCursor += 8;

	if (traceBufferCursor > MEMORY_TRACE_BUFFER_THRESHOLD)
		flushTraceBuffer(false);
}

void MemoryTrace::readWord(unsigned long bankNumber, unsigned long address) {
	traceBuffer[traceBufferCursor++] = 0x02;
	traceBuffer[traceBufferCursor++] = (unsigned char)bankNumber;
	*((unsigned long*)(traceBuffer + traceBufferCursor)) = address;
	traceBufferCursor += 4;
	*((unsigned long long*)(traceBuffer + traceBufferCursor)) = currentCycleNumber;
	traceBufferCursor += 8;

	if (traceBufferCursor > MEMORY_TRACE_BUFFER_THRESHOLD)
		flushTraceBuffer(false);
}

void MemoryTrace::readHalfWord(unsigned long bankNumber, unsigned long address) {
	traceBuffer[traceBufferCursor++] = 0x03;
	traceBuffer[traceBufferCursor++] = (unsigned char)bankNumber;
	*((unsigned long*)(traceBuffer + traceBufferCursor)) = address;
	traceBufferCursor += 4;
	*((unsigned long long*)(traceBuffer + traceBufferCursor)) = currentCycleNumber;
	traceBufferCursor += 8;

	if (traceBufferCursor > MEMORY_TRACE_BUFFER_THRESHOLD)
		flushTraceBuffer(false);
}

void MemoryTrace::readByte(unsigned long bankNumber, unsigned long address) {
	traceBuffer[traceBufferCursor++] = 0x04;
	traceBuffer[traceBufferCursor++] = (unsigned char)bankNumber;
	*((unsigned long*)(traceBuffer + traceBufferCursor)) = address;
	traceBufferCursor += 4;
	*((unsigned long long*)(traceBuffer + traceBufferCursor)) = currentCycleNumber;
	traceBufferCursor += 8;

	if (traceBufferCursor > MEMORY_TRACE_BUFFER_THRESHOLD)
		flushTraceBuffer(false);
}

void MemoryTrace::writeWord(unsigned long bankNumber, unsigned long address) {
	traceBuffer[traceBufferCursor++] = 0x05;
	traceBuffer[traceBufferCursor++] = (unsigned char)bankNumber;
	*((unsigned long*)(traceBuffer + traceBufferCursor)) = address;
	traceBufferCursor += 4;
	*((unsigned long long*)(traceBuffer + traceBufferCursor)) = currentCycleNumber;
	traceBufferCursor += 8;

	if (traceBufferCursor > MEMORY_TRACE_BUFFER_THRESHOLD)
		flushTraceBuffer(false);
}

void MemoryTrace::writeHalfWord(unsigned long bankNumber, unsigned long address) {
	traceBuffer[traceBufferCursor++] = 0x06;
	traceBuffer[traceBufferCursor++] = (unsigned char)bankNumber;
	*((unsigned long*)(traceBuffer + traceBufferCursor)) = address;
	traceBufferCursor += 4;
	*((unsigned long long*)(traceBuffer + traceBufferCursor)) = currentCycleNumber;
	traceBufferCursor += 8;

	if (traceBufferCursor > MEMORY_TRACE_BUFFER_THRESHOLD)
		flushTraceBuffer(false);
}

void MemoryTrace::writeByte(unsigned long bankNumber, unsigned long address) {
	traceBuffer[traceBufferCursor++] = 0x07;
	traceBuffer[traceBufferCursor++] = (unsigned char)bankNumber;
	*((unsigned long*)(traceBuffer + traceBufferCursor)) = address;
	traceBufferCursor += 4;
	*((unsigned long long*)(traceBuffer + traceBufferCursor)) = currentCycleNumber;
	traceBufferCursor += 8;

	if (traceBufferCursor > MEMORY_TRACE_BUFFER_THRESHOLD)
		flushTraceBuffer(false);
}

// Finalize memory trace
void MemoryTrace::stop() {
	flushTraceBuffer(true);
    int err = deflateEnd(&gzipStream);
    assert(err == Z_OK);

	fclose(traceFile);
	traceFile = NULL;

	delete[] traceBuffer;
	delete[] traceCompressBuffer;
}
