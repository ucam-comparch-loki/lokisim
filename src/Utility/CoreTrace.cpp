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

#include "Parameters.h"
#include "CoreTrace.h"

#define CORE_TRACE_BUFFER_SIZE (1024 * 1024)
#define CORE_TRACE_BUFFER_THRESHOLD (CORE_TRACE_BUFFER_SIZE - 1024)

static FILE *traceFile = NULL;

static unsigned char *traceBuffer = NULL;
static unsigned char *traceCompressBuffer = NULL;
static size_t traceBufferCursor = 0;

static unsigned long long currentCycleNumber = 0;

static z_stream gzipStream;

// Addresses and cycle numbers are stored as deltas to improve compression
static unsigned long prevAddress = 0;
static unsigned long long prevCycleNumber = 0;

static void flushTraceBuffer(bool finish) {
	if (traceBufferCursor == 0 && !finish)
		return;

	gzipStream.next_in = (Bytef*)traceBuffer;
	gzipStream.avail_in = (uInt)traceBufferCursor;
	gzipStream.next_out = traceCompressBuffer;
	gzipStream.avail_out = (uInt)CORE_TRACE_BUFFER_SIZE;

    int err = deflate(&gzipStream, finish ? Z_FINISH : Z_NO_FLUSH);
    assert((finish && err == Z_STREAM_END) || (!finish && err == Z_OK));

    if (gzipStream.avail_in > 0)
    	memmove(traceBuffer, traceBuffer + traceBufferCursor - gzipStream.avail_in, gzipStream.avail_in);

    traceBufferCursor = gzipStream.avail_in;
    assert((finish && traceBufferCursor == 0) || (!finish && traceBufferCursor < CORE_TRACE_BUFFER_THRESHOLD));

    size_t bytesOut = gzipStream.next_out - traceCompressBuffer;
	size_t count = fwrite(traceCompressBuffer, 1, bytesOut, traceFile);
	assert(count == bytesOut);
}

static void writeRecord(unsigned long controlByte, unsigned long coreNumber, unsigned long address) {
	traceBuffer[traceBufferCursor++] = (unsigned char)controlByte;
	traceBuffer[traceBufferCursor++] = (unsigned char)coreNumber;

	unsigned long deltaAddress = address - prevAddress;
	signed long signedDeltaAddress = (signed long)deltaAddress;
	unsigned long encodedDeltaAddress = signedDeltaAddress < 0 ? -deltaAddress * 2 + 1 : deltaAddress * 2;
	*((unsigned long*)(traceBuffer + traceBufferCursor)) = encodedDeltaAddress;
	traceBufferCursor += 4;
	prevAddress = address;

	unsigned long long deltaCycleNumber = currentCycleNumber - prevCycleNumber;
	if (deltaCycleNumber < 0xFFULL) {
		traceBuffer[traceBufferCursor++] = (unsigned char)deltaCycleNumber;
	} else {
		traceBuffer[traceBufferCursor++] = (unsigned char)0xFF;
		*((unsigned long long*)(traceBuffer + traceBufferCursor)) = deltaCycleNumber;
		traceBufferCursor += 8;
	}
	prevCycleNumber = currentCycleNumber;

	if (traceBufferCursor > CORE_TRACE_BUFFER_THRESHOLD)
		flushTraceBuffer(false);
}

// Initialize memory tracing engine
void CoreTrace::start(const std::string& filename) {
	traceFile = fopen(filename.c_str(), "wb");

	traceBuffer = new unsigned char[CORE_TRACE_BUFFER_SIZE];
	traceCompressBuffer = new unsigned char[CORE_TRACE_BUFFER_SIZE];
	traceBufferCursor = 0;

	gzipStream.next_in = (Bytef*)traceBuffer;
    gzipStream.avail_in = (uInt)0;
    gzipStream.next_out = traceCompressBuffer;
    gzipStream.avail_out = (uInt)CORE_TRACE_BUFFER_SIZE;

    gzipStream.zalloc = (alloc_func)0;
    gzipStream.zfree = (free_func)0;
    gzipStream.opaque = (voidpf)0;

    int err = deflateInit2(&gzipStream, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 9, Z_DEFAULT_STRATEGY);
    assert(err == Z_OK);

    prevAddress = 0;
    prevCycleNumber = 0;
}

// Update clock cycle
void CoreTrace::setClockCycle(unsigned long long cycleNumber) {
	currentCycleNumber = cycleNumber;
}

// Log core activity
void CoreTrace::decodeInstruction(unsigned long coreNumber, unsigned long address, bool endOfPacket) {
	writeRecord(endOfPacket ? 0x02 : 0x01, coreNumber, address);
}

// Finalize memory trace
void CoreTrace::stop() {
	flushTraceBuffer(true);
    int err = deflateEnd(&gzipStream);
    assert(err == Z_OK);

	fclose(traceFile);
	traceFile = NULL;

	delete[] traceBuffer;
	delete[] traceCompressBuffer;
}
