/*
 * TraceWriter.h
 *
 *  Created on: 6 Oct 2011
 *      Author: afjk2
 */

#ifndef TRACEWRITER_H_
#define TRACEWRITER_H_

#include <assert.h>

#include <string>

#include "../Parameters.h"

class TraceWriter {
public:
	static const size_t kMaximumExtendedRecordDataLength = 4096;
private:
	static const size_t kMaximumRecordLength = kMaximumExtendedRecordDataLength + 18;

	static const size_t kBufferSize = 4 * 1024 * 1024;
	static const size_t kBufferThreshold = kBufferSize - kMaximumRecordLength;
	static const size_t kCompressionBufferSzie = kBufferSize + (kBufferSize / 8) + 4096;

	FILE *mTraceFile;

	unsigned char *mBuffer;
	unsigned char *mCompressionBuffer;
	size_t mBufferCursor;

	unsigned long long mSequenceNumber;

	unsigned long long mCurrentCycleNumber;

	unsigned long mPreviousAddress;
	unsigned long long mPreviousCycleNumber;

	void flushBuffer();
public:
	TraceWriter(const std::string& filename);
	~TraceWriter();

	inline void setClockCycle(unsigned long long cycleNumber) {
		assert(cycleNumber >= mCurrentCycleNumber);
		mCurrentCycleNumber = cycleNumber;
	}

	void writeRecord(unsigned long controlByte, unsigned long componentIndex, unsigned long address);
	void writeExtendedRecord(unsigned long tag, const void *data, unsigned long length);
};

#endif /* TRACEWRITER_H_ */
