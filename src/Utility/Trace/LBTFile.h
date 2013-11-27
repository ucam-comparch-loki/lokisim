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

struct SLBTExtendedDataHeader {
public:
	static const uint64 kSignature = ((uint64)('L'))  // Little endian byte order / LSB first
		| (((uint64)('B')) << 8)
		| (((uint64)('T')) << 16)
		| (((uint64)('$')) << 24)
		| (((uint64)('1')) << 32)
		| (((uint64)('$')) << 40)
		| (((uint64)('2')) << 48)
		| (26ULL << 56);

	static const uint64 kFormatBasicCoreTrace = 1;
	static const uint64 kFormatExtendedCoreTrace = 2;

	uint64 Signature;
	uint64 Format;
	uint64 IndexTableChunkNumber;
	uint64 IndexTableEntryCount;
	uint64 TraceChunkCount;
	uint64 RecordCount;

	uint64 MemorySize;
	uint64 InitialImageIndexChunkNumber;
	uint64 FinalImageIndexChunkNumber;
};

struct SLBTChunkExtendedRecord {
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
	uint32 MemoryData;
};

/*
 * Chunk data layout:
 *
 * 24 byte field transposition in order of fields, delta encoding for cycle numbers and instruction addresses
 */

#pragma pack(pop)

/*
 * Loki binary trace file writer class
 */

class CLBTFileWriter {
private:
	static const usize kChunkRecordCount = 2 * 1024 * 1024;  // 48 MB

	static const usize kIndexTableCapacityInitial = 65536;
	static const usize kIndexTableCapacityIncrement = 32768;
	static const usize kIndexSegmentEntryCount = 8 * 1024 * 1024;  // 64 MB

	static const usize kMemoryImageChunkSize = 64 * 1024 * 1024;  // 64 MB

	CFile &mFile;
	CLBCFFileWriter mWriter;

	uint64 mMemorySize;
	uint64 mInitialImageIndexChunkNumber;
	uint64 mFinalImageIndexChunkNumber;

	CAlignedBuffer mRecordsWrapper;
	SLBTChunkExtendedRecord *mRecords;
	usize mRecordCursor;

	CAlignedBuffer mWorkBuffer;

	CDynamicAlignedBuffer mChunkIndexTableWrapper;
	uint64 *mChunkIndexTable;
	usize mChunkIndexTableCursor;
	usize mChunkIndexTableCapacity;

	CAlignedBuffer mChunkIndexSegmentWrapper;
	uint64 *mChunkIndexSegment;
	usize mChunkIndexSegmentCursor;

	uint64 mTotalRecordCount;
	uint64 mTotalTraceChunkCount;

	void FlushIndexTableSegment();
	void FlushChunkBuffer();
public:
	CLBTFileWriter(CFile &lbtFile);
	~CLBTFileWriter();

	void AddBasicOperation(uint64 cycleNumber, ulong instructionAddress, uint operationType, uint inputChannel1, bool usesInputChannel1, uint inputChannel2, bool usesInputChannel2, bool executed, bool endOfPacket);
	void AddMemoryOperation(uint64 cycleNumber, ulong instructionAddress, uint operationType, ulong memoryAddress, ulong memoryData, bool executed, bool endOfPacket);
	void AddSystemCall(uint64 cycleNumber, ulong instructionAddress, uint systemCallNumber, ulong *registerValues, usize registerCount, const void *data, usize dataLength, bool executed, bool endOfPacket);

	void SetMemorySize(uint64 memorySize);
	void StoreMemoryImage(const void *image, bool initialImage);

	void Flush();
};

#endif /*LBTFILE_H_*/
