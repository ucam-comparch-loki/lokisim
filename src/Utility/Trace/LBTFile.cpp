/*
 * Loki memory analysis and simulation tool
 *
 * Copyright (C) 2010-2013 Andreas Koltes. All rights reserved.
 *
 * Loki binary trace file management
 *
 * Adapted version for stand-alone SystemC simulator integration
 */

#include "LokiTRTLite.h"

#include "LBCFFile.h"
#include "LBTFile.h"

/*
 * Loki binary trace file writer class
 */

void CLBTFileWriter::FlushChunkBuffer() {
	if (mRecordCursor == 0)
		return;

	uint64 *packedCycleNumbers = (uint64*)mWorkBuffer.GetBuffer();
	uint32 *packedInstructionAddresses = (uint32*)(&packedCycleNumbers[mRecordCursor]);
	uint32 *packedMemoryAddresses = (uint32*)(&packedInstructionAddresses[mRecordCursor]);
	uint8 *packedOperationTypes = (uint8*)(&packedMemoryAddresses[mRecordCursor]);
	uint8 *packedParameters1 = (uint8*)(&packedOperationTypes[mRecordCursor]);
	uint8 *packedParameters2 = (uint8*)(&packedParameters1[mRecordCursor]);
	uint8 *packedFlags = (uint8*)(&packedParameters2[mRecordCursor]);
	uint32 *packedMemoryData = (uint32*)(&packedFlags[mRecordCursor]);

	uint64 prevCycleNumber = 0;
	uint32 prevInstructionAddress = 0;

	for (usize i = 0; i < mRecordCursor; i++) {
		packedCycleNumbers[i] = mRecords[i].CycleNumber - prevCycleNumber;
		packedInstructionAddresses[i] = mRecords[i].InstructionAddress - prevInstructionAddress;
		packedMemoryAddresses[i] = mRecords[i].MemoryAddress;
		packedOperationTypes[i] = mRecords[i].OperationType;
		packedParameters1[i] = mRecords[i].Parameter1;
		packedParameters2[i] = mRecords[i].Parameter2;
		packedFlags[i] = mRecords[i].Flags;
		packedMemoryData[i] = mRecords[i].MemoryData;

		prevCycleNumber = mRecords[i].CycleNumber;
		prevInstructionAddress = mRecords[i].InstructionAddress;
	}

	ulong chunkNumber = mWriter.AppendChunk(mWorkBuffer.GetBuffer(), sizeof(SLBTChunkExtendedRecord) * mRecordCursor);
	mTotalRecordCount += mRecordCursor;
	mRecordCursor = 0;

	if (mChunkIndexCursor == mChunkIndexCapacity) {
		mChunkIndexCapacity += kChunkIndexCapacityIncrement;
		mChunkIndexWrapper.Reallocate(sizeof(uint64) * mChunkIndexCapacity);
		mChunkIndex = (uint64*)mChunkIndexWrapper.GetBuffer();
	}

	mChunkIndex[mChunkIndexCursor++] = chunkNumber;
}

CLBTFileWriter::CLBTFileWriter(CFile &lbtFile) :
	mFile(lbtFile),
	mWriter(lbtFile),
	mRecordsWrapper(sizeof(SLBTChunkExtendedRecord) * kChunkRecordCount),
	mWorkBuffer(sizeof(SLBTChunkExtendedRecord) * kChunkRecordCount),
	mChunkIndexWrapper(sizeof(uint64) * kChunkIndexCapacityInitial)
{
	mMemorySize = 0;

	mRecords = (SLBTChunkExtendedRecord*)mRecordsWrapper.GetBuffer();
	mRecordCursor = 0;

	mChunkIndex = (uint64*)mChunkIndexWrapper.GetBuffer();
	mChunkIndexCursor = 0;
	mChunkIndexCapacity = kChunkIndexCapacityInitial;

	mTotalRecordCount = 0;
}

CLBTFileWriter::~CLBTFileWriter() {
	// Nothing
}

void CLBTFileWriter::AddBasicOperation(uint64 cycleNumber, ulong instructionAddress, uint operationType, uint inputChannel1, bool usesInputChannel1, uint inputChannel2, bool usesInputChannel2, bool executed, bool endOfPacket) {
	assert((uint64)instructionAddress + 4 <= mMemorySize);

	if (mRecordCursor == kChunkRecordCount)
		FlushChunkBuffer();

	mRecords[mRecordCursor].CycleNumber = cycleNumber;
	mRecords[mRecordCursor].InstructionAddress = instructionAddress;
	mRecords[mRecordCursor].MemoryAddress = 0;
	mRecords[mRecordCursor].OperationType = operationType;
	mRecords[mRecordCursor].Parameter1 = usesInputChannel1 ? inputChannel1 : 0;
	mRecords[mRecordCursor].Parameter2 = usesInputChannel2 ? inputChannel2 : 0;

	mRecords[mRecordCursor].Flags = 0;
	if (usesInputChannel1)
		mRecords[mRecordCursor].Flags |= SLBTChunkExtendedRecord::kFlagInputChannel1;
	if (usesInputChannel2)
		mRecords[mRecordCursor].Flags |= SLBTChunkExtendedRecord::kFlagInputChannel2;
	if (!executed)
		mRecords[mRecordCursor].Flags |= SLBTChunkExtendedRecord::kFlagNotExecuted;
	if (endOfPacket)
		mRecords[mRecordCursor].Flags |= SLBTChunkExtendedRecord::kFlagEndOfPacket;

	mRecords[mRecordCursor].MemoryData = 0;

	mRecordCursor++;
}

void CLBTFileWriter::AddMemoryOperation(uint64 cycleNumber, ulong instructionAddress, uint operationType, ulong memoryAddress, ulong memoryData, bool executed, bool endOfPacket) {
	assert((uint64)instructionAddress + 4 <= mMemorySize);
	assert((uint64)memoryAddress + 4 <= mMemorySize);

	if (mRecordCursor == kChunkRecordCount)
		FlushChunkBuffer();

	mRecords[mRecordCursor].CycleNumber = cycleNumber;
	mRecords[mRecordCursor].InstructionAddress = instructionAddress;
	mRecords[mRecordCursor].MemoryAddress = memoryAddress;
	mRecords[mRecordCursor].OperationType = operationType;
	mRecords[mRecordCursor].Parameter1 = 0;
	mRecords[mRecordCursor].Parameter2 = 0;

	mRecords[mRecordCursor].Flags = 0;
	if (!executed)
		mRecords[mRecordCursor].Flags |= SLBTChunkExtendedRecord::kFlagNotExecuted;
	if (endOfPacket)
		mRecords[mRecordCursor].Flags |= SLBTChunkExtendedRecord::kFlagEndOfPacket;

	mRecords[mRecordCursor].MemoryData = memoryData;

	mRecordCursor++;
}

void CLBTFileWriter::AddSystemCall(uint64 cycleNumber, ulong instructionAddress, uint systemCallNumber, ulong *registerValues, usize registerCount, const void *data, usize dataLength, bool executed, bool endOfPacket) {
	assert((uint64)instructionAddress + 4 <= mMemorySize);
	assert(data != NULL || dataLength == 0);

	if (mRecordCursor == kChunkRecordCount)
		FlushChunkBuffer();

	usize bufferSize = sizeof(uint32) * (registerCount + 1) + dataLength;
	CAlignedBuffer tempBuffer(bufferSize);
	uint8 *buffer = (uint8*)(tempBuffer.GetBuffer());

	((uint32*)buffer)[0] = registerCount;
	for (usize i = 0; i < registerCount; i++)
		((uint32*)buffer)[i + 1] = registerValues[i];

	if (dataLength > 0)
		memcpy(&((uint32*)buffer)[registerCount + 1], data, dataLength);

	ulong sysCallChunk = mWriter.AppendChunk(buffer, bufferSize);

	mRecords[mRecordCursor].CycleNumber = cycleNumber;
	mRecords[mRecordCursor].InstructionAddress = instructionAddress;
	mRecords[mRecordCursor].MemoryAddress = sysCallChunk;
	mRecords[mRecordCursor].OperationType = SLBTChunkExtendedRecord::kOperationTypeSystemCall;
	mRecords[mRecordCursor].Parameter1 = systemCallNumber;
	mRecords[mRecordCursor].Parameter2 = 0;

	mRecords[mRecordCursor].Flags = 0;
	if (!executed)
		mRecords[mRecordCursor].Flags |= SLBTChunkExtendedRecord::kFlagNotExecuted;
	if (endOfPacket)
		mRecords[mRecordCursor].Flags |= SLBTChunkExtendedRecord::kFlagEndOfPacket;

	mRecords[mRecordCursor].MemoryData = 0;

	mRecordCursor++;
}

void CLBTFileWriter::SetMemorySize(uint64 memorySize) {
	assert(memorySize > 0);
	mMemorySize = memorySize;
}

void CLBTFileWriter::StoreMemoryImage(const void *image, bool initialImage) {
	assert(mMemorySize > 0);

	usize chunkCount = mMemorySize / kMemoryImageChunkSize;
	if (mMemorySize % kMemoryImageChunkSize != 0)
		chunkCount++;

	uint64 *chunkIndexes = new uint64[chunkCount];
	uint64 bytesLeft = mMemorySize;
	const uint8 *cursor = (const uint8*)image;

	for (usize i = 0; i < chunkCount; i++) {
		usize byteCount = bytesLeft > kMemoryImageChunkSize ? kMemoryImageChunkSize : (usize)bytesLeft;
		bytesLeft -= byteCount;

		chunkIndexes[i] = mWriter.AppendChunk(cursor, byteCount);
		cursor += byteCount;
	}

	usize indexChunkIndex = mWriter.AppendChunk(chunkIndexes, sizeof(uint64) * chunkCount);
	delete[] chunkIndexes;

	if (initialImage)
		mInitialImageIndexChunkNumber = indexChunkIndex;
	else
		mFinalImageIndexChunkNumber = indexChunkIndex;
}

void CLBTFileWriter::Flush() {
	FlushChunkBuffer();

	ulong indexChunkNumber = mWriter.AppendChunk(mChunkIndex, sizeof(uint64) * mChunkIndexCursor);

	SLBTExtendedDataHeader header;
	header.Signature = SLBTExtendedDataHeader::kSignature;
	header.Format = SLBTExtendedDataHeader::kFormatExtendedCoreTrace;
	header.IndexChunkNumber = indexChunkNumber;
	header.TraceChunkCount = mChunkIndexCursor;
	header.RecordCount = mTotalRecordCount;
	header.MemorySize = mMemorySize;
	header.InitialImageIndexChunkNumber = mInitialImageIndexChunkNumber;
	header.FinalImageIndexChunkNumber = mFinalImageIndexChunkNumber;

	mWriter.SetUserData(&header, sizeof(SLBTExtendedDataHeader));
	mWriter.Flush();
}
