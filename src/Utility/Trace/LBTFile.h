/*
 * Loki memory analysis and simulation tool
 *
 * Copyright (C) 2010-2013 Andreas Koltes. All rights reserved.
 *
 * Loki binary trace file management
 *
 * Adapted version for stand-alone SystemC simulator integration
 */

#ifndef LBTFILE_H_
#define LBTFILE_H_

#include "LokiTRTLite.h"

#include "LBCFFile.h"

/*
 * Loki binary trace file format definitions
 */

#pragma pack(push)
#pragma pack(1)

struct SLBTDataHeader {
public:
	static const uint64 kSignature = ((uint64)('L'))  // Little endian byte order / LSB first
		| (((uint64)('B')) << 8)
		| (((uint64)('T')) << 16)
		| (((uint64)('$')) << 24)
		| (((uint64)('1')) << 32)
		| (((uint64)('$')) << 40)
		| (((uint64)('0')) << 48)
		| (26ULL << 56);

	static const uint32 kFormatBasicCoreTrace = 1;

	uint64 Signature;
	uint32 Format;
	uint64 IndexChunkNumber;
	uint64 TraceChunkCount;
	uint64 RecordCount;
};

struct SLBTChunkRecord {
public:
	static const uint8 kOperationTypeNOP = 1;
	static const uint8 kOperationTypeALU1 = 2;
	static const uint8 kOperationTypeALU2 = 3;
	static const uint8 kOperationTypeFetch = 4;
	static const uint8 kOperationTypeScratchpadRead = 5;
	static const uint8 kOperationTypeScratchpadWrite = 6;
	static const uint8 kOperationTypeLoadImmediate = 7;
	static const uint8 kOperationTypeSystemCall = 8;
	static const uint8 kOperationTypeControl = 9;
	static const uint8 kOperationTypeLoadWord = 10;
	static const uint8 kOperationTypeLoadHalfWord = 11;
	static const uint8 kOperationTypeLoadByte = 12;
	static const uint8 kOperationTypeStoreWord = 13;
	static const uint8 kOperationTypeStoreHalfWord = 14;
	static const uint8 kOperationTypeStoreByte = 15;

	static const uint8 kFlagEndOfPacket = 0x1;
	static const uint8 kFlagInputChannel1 = 0x2;
	static const uint8 kFlagInputChannel2 = 0x4;
	static const uint8 kFlagNotExecuted = 0x8;

	uint64 CycleNumber;
	uint32 InstructionAddress;
	uint32 MemoryAddress;		// Memory address or scratchpad slot or chunk number holding system call information
	uint8 OperationType;
	uint8 Parameter1;			// Input channel 1 or system call number
	uint8 Parameter2;			// Input channel 2 or high 8 bits of chunk number holding system call information
	uint8 Flags;
};

/*
 * Chunk data layout:
 *
 * 20 byte field transposition in order of fields, delta encoding for cycle numbers and instruction addresses
 */

#pragma pack(pop)

/*
 * Loki binary trace file writer class
 */

class CLBTFileReader;

class CLBTFileWriter {
private:
	static const usize kChunkRecordCount = 2 * 1024 * 1024;  // 40 MB

	static const usize kChunkIndexCapacityInitial = 65536;
	static const usize kChunkIndexCapacityIncrement = 32768;

	CFile &mFile;
	CLBCFFileWriter mWriter;

	CAlignedBuffer mRecordsWrapper;
	SLBTChunkRecord *mRecords;
	usize mRecordCursor;

	CAlignedBuffer mWorkBuffer;

	CDynamicAlignedBuffer mChunkIndexWrapper;
	uint64 *mChunkIndex;
	usize mChunkIndexCursor;
	usize mChunkIndexCapacity;

	uint64 mTotalRecordCount;

	void FlushChunkBuffer();
public:
	CLBTFileWriter(CFile &lbtFile);
	~CLBTFileWriter();

	void AddBasicOperation(uint64 cycleNumber, ulong instructionAddress, uint operationType, uint inputChannel1, bool usesInputChannel1, uint inputChannel2, bool usesInputChannel2, bool executed, bool endOfPacket);
	void AddMemoryOperation(uint64 cycleNumber, ulong instructionAddress, uint operationType, ulong memoryAddress, bool executed, bool endOfPacket);
	void AddSystemCall(uint64 cycleNumber, ulong instructionAddress, uint systemCallNumber, ulong *registerValues, usize registerCount, const void *data, usize dataLength, bool executed, bool endOfPacket);

	void Flush();
};

/*
 * Loki binary trace file reader class
 */

class CLBTFileReader {
	friend class CLBTFileWriter;
private:
	CFile &mFile;
	CLBCFFileReader mReader;

	CDynamicAlignedBuffer mChunkIndexWrapper;
	uint64 *mTraceChunkNumbers;
	usize mTraceChunkIndex;
	usize mTraceChunkCount;

	CDynamicAlignedBuffer mChunkDataBuffer;
	uint64 *mChunkCycleNumbers;
	uint32 *mChunkInstructionAddresses;
	uint32 *mChunkMemoryAddresses;
	uint8 *mChunkOperationTypes;
	uint8 *mChunkParameters1;
	uint8 *mChunkParameters2;
	uint8 *mChunkFlags;

	usize mChunkRecordCursor;
	usize mChunkRecordCount;

	uint64 mTotalRecordCount;

	bool ReadChunk();
public:
	CLBTFileReader(CFile &lbtFile);
	~CLBTFileReader();

	inline void Rewind() {
		mTraceChunkIndex = (usize)(-1LL);
		mChunkRecordCursor = 0;
		mChunkRecordCount = 0;
	}

	inline bool EndOfFile() const {
		return mTraceChunkIndex == mTraceChunkCount;
	}

	inline bool Read(SLBTChunkRecord &record) {
		if (mChunkRecordCursor == mChunkRecordCount && !ReadChunk())
			return false;

		record.CycleNumber = mChunkCycleNumbers[mChunkRecordCursor];
		record.InstructionAddress = mChunkInstructionAddresses[mChunkRecordCursor];
		record.MemoryAddress = mChunkMemoryAddresses[mChunkRecordCursor];
		record.OperationType = mChunkOperationTypes[mChunkRecordCursor];
		record.Parameter1 = mChunkParameters1[mChunkRecordCursor];
		record.Parameter2 = mChunkParameters2[mChunkRecordCursor];
		record.Flags = mChunkFlags[mChunkRecordCursor];

		mChunkRecordCursor++;

		return true;
	}

	inline const char* GetFileName() const			{ return mReader.GetFileName(); }

	inline uint64 GetRecordCount() const			{ return mTotalRecordCount; }
};

#endif /*LBTFILE_H_*/
