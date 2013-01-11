/*
 * TraceWriter.cpp
 *
 *  Created on: 6 Oct 2011
 *      Author: afjk2
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include <zlib.h>

#include "../Parameters.h"
#include "TraceWriter.h"

#pragma pack(push)
#pragma pack(1)

struct TraceDataPacketHeader {
public:
	static const unsigned int kSignaturePacket = 0x31304454;
	static const unsigned int kSignatureTerminator = 0x32304454;

	unsigned int Singature;
	unsigned int UncompressedSize;
	unsigned int CompressedSize;
	unsigned int Checksum;
	unsigned long long SequenceNumber;
};

#pragma pack(pop)

void TraceWriter::flushBuffer() {
	assert(mBufferCursor > 0);

	z_stream stream;

	stream.next_in = (Bytef*)mBuffer;
	stream.avail_in = (uInt)mBufferCursor;
	stream.next_out = mCompressionBuffer;
	stream.avail_out = (uInt)kCompressionBufferSzie;

	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;

    int err = deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
    assert(err == Z_OK);

    err = deflate(&stream, Z_FINISH);
    assert(err == Z_STREAM_END);
    assert(stream.avail_in == 0);

    size_t bytesOut = stream.next_out - mCompressionBuffer;

    TraceDataPacketHeader header;
    header.Singature = TraceDataPacketHeader::kSignaturePacket;
    header.UncompressedSize = mBufferCursor;
    header.CompressedSize = bytesOut;
    header.Checksum = crc32(crc32(0L, Z_NULL, 0), mBuffer, mBufferCursor);
    header.SequenceNumber = mSequenceNumber++;

    size_t count = fwrite(&header, 1, sizeof(header), mTraceFile);
	assert(count == sizeof(header));
    count = fwrite(mCompressionBuffer, 1, bytesOut, mTraceFile);
	assert(count == bytesOut);

	mBufferCursor = 0;

	mPreviousAddress = 0;
	mPreviousCycleNumber = 0;
}

TraceWriter::TraceWriter(const std::string& filename) {
	mTraceFile = fopen(filename.c_str(), "wb");

	mBuffer = new unsigned char[kBufferSize];
	mCompressionBuffer = new unsigned char[kCompressionBufferSzie];
	mBufferCursor = 0;

	mSequenceNumber = 0;

	mCurrentCycleNumber = 0;

	mPreviousAddress = 0;
	mPreviousCycleNumber = 0;
}

TraceWriter::~TraceWriter() {
	if (mBufferCursor > 0)
		flushBuffer();

    TraceDataPacketHeader header;
    header.Singature = TraceDataPacketHeader::kSignatureTerminator;
    header.UncompressedSize = 0;
    header.CompressedSize = 0;
    header.Checksum = crc32(0L, Z_NULL, 0);
    header.SequenceNumber = mSequenceNumber++;

    size_t count = fwrite(&header, 1, sizeof(header), mTraceFile);
	assert(count == sizeof(header));

	fclose(mTraceFile);

	delete[] mBuffer;
	delete[] mCompressionBuffer;
}

/* Trace data format (MSB first / big endian):
 *
 * Initial control byte:
 *
 * 0x00 - 0x7E: Control byte followed by component index byte
 * 0x7F:        Extended record
 * 0x80 - 0xFF: 1 flag bit, 3 component index bits, 4 control bits (high to low)
 *
 * Following address / Cycle encoding byte:
 *
 * High 5 bits - address encoding (zero for extended record):
 *
 * 0x00 - Plain 4 byte address
 * 0x01 - Previous address repetition
 * 0x02 - Previous address + 1 byte
 * ...
 * 0x09 - Previous address + 8 bytes
 * 0x0A - Previous address - 1 byte
 * ...
 * 0x11 - Previous address - 8 bytes
 * 0x12 - Previous address + 1 byte offset
 * 0x13 - Previous address + 2 byte offset
 * 0x14 - Previous address + 3 byte offset
 * 0x15 - Previous address - 1 byte offset
 * 0x16 - Previous address - 2 byte offset
 * 0x17 - Previous address - 3 byte offset
 *
 * Low 3 bits - cycle number encoding:
 *
 * 0x0 - Plain 8 byte cycle number
 * 0x1 - Previous cycle number
 * 0x2 - Previous cycle number + 1
 * 0x3 - Previous cycle number + 2
 * 0x4 - Previous cycle number + 3
 * 0x5 - Previous cycle number + 4
 * 0x6 - Previous cycle number + 1 byte offset
 * 0x7 - Previous cycle number + 2 byte offset
 *
 * For extended record:
 *
 * 4 tag bytes, 4 length bytes, data following
 */

void TraceWriter::writeRecord(unsigned long controlByte, unsigned long componentIndex, unsigned long address) {
	assert(mBufferCursor < kBufferThreshold);
	assert(controlByte <= 0x7E);
	assert(componentIndex <= 0xFF);

	// Encode control byte and component index

	if (controlByte <= 0xF && componentIndex <= 0x7) {
		mBuffer[mBufferCursor++] = 0x80 | (componentIndex << 4) | controlByte;
	} else {
		mBuffer[mBufferCursor++] = controlByte;
		mBuffer[mBufferCursor++] = componentIndex;
	}

	size_t encodingByteOffset = mBufferCursor++;

	// Encode address

	unsigned int addressEncoding = 0x00;

	if (address == mPreviousAddress) {
		addressEncoding = 0x01;
	} else if (address > mPreviousAddress) {
		unsigned long delta = address - mPreviousAddress;
		if (delta <= 8UL) {
			addressEncoding = delta + 0x01;
		} else if (delta <= 0xFFUL) {
			addressEncoding = 0x12;
			mBuffer[mBufferCursor++] = delta;
		} else if (delta <= 0xFFFFUL) {
			addressEncoding = 0x13;
			mBuffer[mBufferCursor++] = delta >> 8;
			mBuffer[mBufferCursor++] = delta & 0xFFUL;
		} else if (delta <= 0xFFFFFFUL) {
			addressEncoding = 0x14;
			mBuffer[mBufferCursor++] = delta >> 16;
			mBuffer[mBufferCursor++] = (delta >> 8) & 0xFFUL;
			mBuffer[mBufferCursor++] = delta & 0xFFUL;
		}
	} else if (address < mPreviousAddress) {
		unsigned long delta = mPreviousAddress - address;
		if (delta <= 8UL) {
			addressEncoding = delta + 0x09;
		} else if (delta <= 0xFFUL) {
			addressEncoding = 0x15;
			mBuffer[mBufferCursor++] = delta;
		} else if (delta <= 0xFFFFUL) {
			addressEncoding = 0x16;
			mBuffer[mBufferCursor++] = delta >> 8;
			mBuffer[mBufferCursor++] = delta & 0xFFUL;
		} else if (delta <= 0xFFFFFFUL) {
			addressEncoding = 0x17;
			mBuffer[mBufferCursor++] = delta >> 16;
			mBuffer[mBufferCursor++] = (delta >> 8) & 0xFFUL;
			mBuffer[mBufferCursor++] = delta & 0xFFUL;
		}
	}

	if (addressEncoding == 0x00) {
		mBuffer[mBufferCursor++] = address >> 24;
		mBuffer[mBufferCursor++] = (address >> 16) & 0xFFUL;
		mBuffer[mBufferCursor++] = (address >> 8) & 0xFFUL;
		mBuffer[mBufferCursor++] = address & 0xFFUL;
	}

	mPreviousAddress = address;

	// Encode cycle number

	unsigned int cycleNumberEncoding = 0x0;

	if (mCurrentCycleNumber == mPreviousCycleNumber) {
		cycleNumberEncoding = 0x01;
	} else {
		unsigned long long delta = mCurrentCycleNumber - mPreviousCycleNumber;
		if (delta <= 4ULL) {
			cycleNumberEncoding = delta + 0x1;
		} else if (delta <= 0xFFULL) {
			cycleNumberEncoding = 0x6;
			mBuffer[mBufferCursor++] = delta;
		} else if (delta <= 0xFFFFULL) {
			cycleNumberEncoding = 0x7;
			mBuffer[mBufferCursor++] = delta >> 8;
			mBuffer[mBufferCursor++] = delta & 0xFFULL;
		}
		// Flush buffer if necessary

		if (mBufferCursor >= kBufferThreshold)
			flushBuffer();

	}

	if (cycleNumberEncoding == 0x0) {
		mBuffer[mBufferCursor++] = mCurrentCycleNumber >> 56;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 48) & 0xFFULL;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 40) & 0xFFULL;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 32) & 0xFFULL;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 24) & 0xFFULL;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 16) & 0xFFULL;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 8) & 0xFFULL;
		mBuffer[mBufferCursor++] = mCurrentCycleNumber & 0xFFULL;
	}

	mPreviousCycleNumber = mCurrentCycleNumber;

	// Store encoding indicator

	mBuffer[encodingByteOffset] = (addressEncoding << 3) | cycleNumberEncoding;

	// Flush buffer if necessary

	if (mBufferCursor >= kBufferThreshold)
		flushBuffer();
}

void TraceWriter::writeExtendedRecord(unsigned long tag, const void *data, unsigned long length) {
	assert(mBufferCursor < kBufferThreshold);
	assert(data != NULL);
	assert(length <= kMaximumExtendedRecordDataLength);

	// Encode control byte

	mBuffer[mBufferCursor++] = 0x7F;

	size_t encodingByteOffset = mBufferCursor++;

	// Encode cycle number

	unsigned int cycleNumberEncoding = 0x0;

	if (mCurrentCycleNumber == mPreviousCycleNumber) {
		cycleNumberEncoding = 0x01;
	} else {
		unsigned long long delta = mCurrentCycleNumber - mPreviousCycleNumber;
		if (delta <= 4ULL) {
			cycleNumberEncoding = delta + 0x1;
		} else if (delta <= 0xFFULL) {
			cycleNumberEncoding = 0x6;
			mBuffer[mBufferCursor++] = delta;
		} else if (delta <= 0xFFFFULL) {
			cycleNumberEncoding = 0x7;
			mBuffer[mBufferCursor++] = delta >> 8;
			mBuffer[mBufferCursor++] = delta & 0xFFULL;
		}
	}

	if (cycleNumberEncoding == 0x0) {
		mBuffer[mBufferCursor++] = mCurrentCycleNumber >> 56;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 48) & 0xFFULL;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 40) & 0xFFULL;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 32) & 0xFFULL;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 24) & 0xFFULL;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 16) & 0xFFULL;
		mBuffer[mBufferCursor++] = (mCurrentCycleNumber >> 8) & 0xFFULL;
		mBuffer[mBufferCursor++] = mCurrentCycleNumber & 0xFFULL;
	}

	mPreviousCycleNumber = mCurrentCycleNumber;

	// Store encoding indicator

	mBuffer[encodingByteOffset] = cycleNumberEncoding;

	// Store payload data

	mBuffer[mBufferCursor++] = (tag >> 24) & 0xFFULL;
	mBuffer[mBufferCursor++] = (tag >> 16) & 0xFFULL;
	mBuffer[mBufferCursor++] = (tag >> 8) & 0xFFULL;
	mBuffer[mBufferCursor++] = tag & 0xFFULL;

	mBuffer[mBufferCursor++] = (length >> 24) & 0xFFULL;
	mBuffer[mBufferCursor++] = (length >> 16) & 0xFFULL;
	mBuffer[mBufferCursor++] = (length >> 8) & 0xFFULL;
	mBuffer[mBufferCursor++] = length & 0xFFULL;

	memcpy(&mBuffer[mBufferCursor], data, length);
	mBufferCursor += length;

	// Flush buffer if necessary

	if (mBufferCursor >= kBufferThreshold)
		flushBuffer();
}
