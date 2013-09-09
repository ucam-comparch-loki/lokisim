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

		prevCycleNumber = mRecords[i].CycleNumber;
		prevInstructionAddress = mRecords[i].InstructionAddress;
	}

	ulong chunkNumber = mWriter.AppendChunk(mWorkBuffer.GetBuffer(), sizeof(SLBTChunkRecord) * mRecordCursor);
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
	mRecordsWrapper(sizeof(SLBTChunkRecord) * kChunkRecordCount),
	mWorkBuffer(sizeof(SLBTChunkRecord) * kChunkRecordCount),
	mChunkIndexWrapper(sizeof(uint64) * kChunkIndexCapacityInitial)
{
	mRecords = (SLBTChunkRecord*)mRecordsWrapper.GetBuffer();
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
		mRecords[mRecordCursor].Flags |= SLBTChunkRecord::kFlagInputChannel1;
	if (usesInputChannel2)
		mRecords[mRecordCursor].Flags |= SLBTChunkRecord::kFlagInputChannel2;
	if (!executed)
		mRecords[mRecordCursor].Flags |= SLBTChunkRecord::kFlagNotExecuted;
	if (endOfPacket)
		mRecords[mRecordCursor].Flags |= SLBTChunkRecord::kFlagEndOfPacket;

	mRecordCursor++;
}

void CLBTFileWriter::AddMemoryOperation(uint64 cycleNumber, ulong instructionAddress, uint operationType, ulong memoryAddress, bool executed, bool endOfPacket) {
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
		mRecords[mRecordCursor].Flags |= SLBTChunkRecord::kFlagNotExecuted;
	if (endOfPacket)
		mRecords[mRecordCursor].Flags |= SLBTChunkRecord::kFlagEndOfPacket;

	mRecordCursor++;
}

void CLBTFileWriter::AddSystemCall(uint64 cycleNumber, ulong instructionAddress, uint systemCallNumber, ulong *registerValues, usize registerCount, const void *data, usize dataLength, bool executed, bool endOfPacket) {
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
	mRecords[mRecordCursor].OperationType = SLBTChunkRecord::kOperationTypeSystemCall;
	mRecords[mRecordCursor].Parameter1 = systemCallNumber;
	mRecords[mRecordCursor].Parameter2 = 0;

	mRecords[mRecordCursor].Flags = 0;
	if (!executed)
		mRecords[mRecordCursor].Flags |= SLBTChunkRecord::kFlagNotExecuted;
	if (endOfPacket)
		mRecords[mRecordCursor].Flags |= SLBTChunkRecord::kFlagEndOfPacket;

	mRecordCursor++;
}

void CLBTFileWriter::Flush() {
	ulong indexChunkNumber = mWriter.AppendChunk(mChunkIndex, sizeof(uint64) * mChunkIndexCursor);

	SLBTDataHeader header;
	header.Signature = SLBTDataHeader::kSignature;
	header.Format = SLBTDataHeader::kFormatBasicCoreTrace;
	header.IndexChunkNumber = indexChunkNumber;
	header.TraceChunkCount = mChunkIndexCursor;
	header.RecordCount = mTotalRecordCount;

	mWriter.SetUserData(&header, sizeof(SLBTDataHeader));
	mWriter.Flush();
}

/*
 * Loki binary trace file reader class
 */

bool CLBTFileReader::ReadChunk() {
	// Check for end of file

	if (mTraceChunkIndex + 1 == mTraceChunkCount)
		return false;

	mTraceChunkIndex++;

	// Allocate buffer if necessary and read chunk data

	usize chunkSize = mReader.GetChunkSize(mTraceChunkIndex);

	if (chunkSize % sizeof(SLBTChunkRecord) != 0)
		gExceptionManager.RaiseException(CException::LogicalFormat, "Invalid trace data chunk size", mReader.GetFileName());

	usize recordCount = chunkSize / sizeof(SLBTChunkRecord);

	if (mChunkDataBuffer.GetSize() < chunkSize)
		mChunkDataBuffer.Allocate(chunkSize);

	uint8 *chunkBuffer = (uint8*)mChunkDataBuffer.GetBuffer();

	mReader.ReadChunk(mTraceChunkIndex, chunkBuffer, chunkSize);

	// Initialise chunk data pointers

	mChunkCycleNumbers = (uint64*)chunkBuffer;
	mChunkInstructionAddresses = (uint32*)(&mChunkCycleNumbers[recordCount]);
	mChunkMemoryAddresses = (uint32*)(&mChunkInstructionAddresses[recordCount]);
	mChunkOperationTypes = (uint8*)(&mChunkMemoryAddresses[recordCount]);
	mChunkParameters1 = (uint8*)(&mChunkOperationTypes[recordCount]);
	mChunkParameters2 = (uint8*)(&mChunkParameters1[recordCount]);
	mChunkFlags = (uint8*)(&mChunkParameters2[recordCount]);

	// Decode delta encoded data

	uint64 prevCycleNumber = 0;
	uint32 prevInstructionAddress = 0;

	for (usize i = 0; i < recordCount; i++) {
		mChunkCycleNumbers[i] += prevCycleNumber;
		mChunkInstructionAddresses[i] += prevInstructionAddress;

		prevCycleNumber = mChunkCycleNumbers[i];
		prevInstructionAddress = mChunkInstructionAddresses[i];
	}

	return true;
}

CLBTFileReader::CLBTFileReader(CFile &lbtFile) :
	mFile(lbtFile),
	mReader(lbtFile),
	mChunkIndexWrapper(),
	mChunkDataBuffer()
{
	// Read and check descriptor

	const uint8 *data = (const uint8*)mReader.GetUserData();
	usize dataSize = mReader.GetUserDataSize();

	if (dataSize != sizeof(SLBTDataHeader))
		gExceptionManager.RaiseException(CException::FileFormat, "Invalid LBT file format", mFile.GetFileName());

	const SLBTDataHeader *header = (const SLBTDataHeader*)data;

	if (header->Signature != SLBTDataHeader::kSignature || header->TraceChunkCount > 0x10000000)
		gExceptionManager.RaiseException(CException::FileFormat, "Invalid LBT file format", mFile.GetFileName());

	mTotalRecordCount = header->RecordCount;

	// Read and check trace chunk index

	usize chunkSize = sizeof(uint64) * header->TraceChunkCount;

	mChunkIndexWrapper.Allocate(chunkSize);
	mTraceChunkNumbers = (uint64*)mChunkIndexWrapper.GetBuffer();

	if (mReader.GetChunkSize(header->IndexChunkNumber) != chunkSize)
		gExceptionManager.RaiseException(CException::LogicalFormat, "Invalid trace data index chunk size", mReader.GetFileName());

	mReader.ReadChunk(header->IndexChunkNumber, mTraceChunkNumbers, chunkSize);

	// Initialize chunk state

	mTraceChunkIndex = (usize)(-1LL);
	mTraceChunkCount = header->TraceChunkCount;
	mChunkRecordCursor = 0;
	mChunkRecordCount = 0;
}

CLBTFileReader::~CLBTFileReader() {
	// Nothing
}
