/*
 * Loki memory analysis and simulation tool
 *
 * Copyright (C) 2010-2013 Andreas Koltes. All rights reserved.
 *
 * Loki binary container file management
  *
 * Adapted version for stand-alone SystemC simulator integration
 */

#include "LokiTRTLite.h"

#include "LBCFFile.h"

/*
 * Loki binary container file writer class
 */

void CLBCFFileWriter::FlushChunkTableSegment() {
	// Do not flush empty chunk table segments

	if (mChunkTableSegmentCursor == 0)
		return;

	usize segmentSize = sizeof(SLBCFChunkTableEntry) * mChunkTableSegmentCursor;

	// Allocate work buffer if necessary

	usize minWorkBufferSize = CDeflate::GetBound(segmentSize, CDeflate::kMinimumCompressionLevel);
	if (mWorkBufferWrapper.GetSize() < minWorkBufferSize) {
		mWorkBufferWrapper.Allocate(minWorkBufferSize);
		mWorkBuffer = (uint8*)mWorkBufferWrapper.GetBuffer();
	}

	// Compress chunk table segment data

	ulong checksum = CCRC32::CalculateCRC32(mChunkTableSegment, segmentSize);
	usize compressedSize;

	if (!CDeflate::Compress(mChunkTableSegment, segmentSize, mWorkBuffer, minWorkBufferSize, compressedSize, CDeflate::kMinimumCompressionLevel))
		gExceptionManager.RaiseException(CException::Generic, "Error compressing chunk table segment data");

	// Expand chunk table index buffer if necessary

	if (mChunkTableIndexCursor == mChunkTableIndexCapacity) {
		mChunkTableIndexCapacity += kChunkTableIndexCapacityIncrement;
		mChunkTableIndexWrapper.Reallocate(sizeof(SLBCFChunkTableIndexEntry) * mChunkTableIndexCapacity);
		mChunkTableIndex = (SLBCFChunkTableIndexEntry*)mChunkTableIndexWrapper.GetBuffer();
	}

	// Allocate chunk table index entry

	assert(mChunkTableIndexCursor < mChunkTableIndexCapacity);
	mChunkTableIndex[mChunkTableIndexCursor].Offset = mFileSize;
	mChunkTableIndex[mChunkTableIndexCursor].EntryCount = mChunkTableSegmentCursor;
	mChunkTableIndex[mChunkTableIndexCursor].SizeCompressed = compressedSize;
	mChunkTableIndex[mChunkTableIndexCursor].Checksum = checksum;
	mChunkTableIndexCursor++;

	// Write data to LBCF file

	mFile.Write(mChunkTableSegment, (uint32)segmentSize);
	mFileSize += segmentSize;

	// Reset chunk table segment cursor

	mChunkTableSegmentCursor = 0;
}

CLBCFFileWriter::CLBCFFileWriter(CFile &lbcfFile) :
	mFile(lbcfFile),
	mFileSize(sizeof(SLBCFFileHeader)),
	mWorkBufferWrapper(),
	mWorkBuffer(NULL),
	mChunkTableIndexWrapper(sizeof(SLBCFChunkTableIndexEntry) * kChunkTableIndexCapacityInitial),
	mChunkTableSegmentWrapper(sizeof(SLBCFChunkTableEntry) * kChunkTableSegmentEntryCount),
	mUserDataWrapper(),
	mUserData(NULL),
	mUserDataSize(0)
{
	mChunkTableIndex = (SLBCFChunkTableIndexEntry*)mChunkTableIndexWrapper.GetBuffer();
	mChunkTableIndexCursor = 0;
	mChunkTableIndexCapacity = kChunkTableIndexCapacityInitial;

	mChunkTableSegment = (SLBCFChunkTableEntry*)mChunkTableSegmentWrapper.GetBuffer();
	mChunkTableSegmentCursor = 0;

	mTotalChunkCount = 0;

	lbcfFile.SetSize(sizeof(SLBCFFileHeader));
	lbcfFile.SetAbsolutePosition(sizeof(SLBCFFileHeader));
}

CLBCFFileWriter::~CLBCFFileWriter() {
	// Nothing
}

void CLBCFFileWriter::SetUserData(const void *buffer, usize length) {
	assert(buffer != NULL || length == 0);

	if (length == 0) {
		mUserDataWrapper.Release();
		mUserData = NULL;
		mUserDataSize = 0;
	} else {
		mUserDataWrapper.Allocate(length);
		mUserData = mUserDataWrapper.GetBuffer();
		mUserDataSize = length;
		memcpy(mUserData, buffer, length);
	}
}

uint64 CLBCFFileWriter::AppendChunk(const void *buffer, usize length) {
	assert(buffer != NULL);
	assert(length <= kMaximumChunkSize);

	// Flush chunk table segment if necessary - overwrites work buffer

	if (mChunkTableSegmentCursor == kChunkTableSegmentEntryCount)
		FlushChunkTableSegment();

	// Allocate work buffer if necessary

	usize minWorkBufferSize = CDeflate::GetBound(length, CDeflate::kMinimumCompressionLevel) + sizeof(uint32);
	if (mWorkBufferWrapper.GetSize() < minWorkBufferSize) {
		mWorkBufferWrapper.Allocate(minWorkBufferSize);
		mWorkBuffer = (uint8*)mWorkBufferWrapper.GetBuffer();
	}

	// Compress chunk data

	ulong checksum = CCRC32::CalculateCRC32(buffer, length);
	usize compressedSize;

	if (!CDeflate::Compress(buffer, length, mWorkBuffer, minWorkBufferSize, compressedSize, CDeflate::kMinimumCompressionLevel))
		gExceptionManager.RaiseException(CException::Generic, "Error compressing chunk data");

	*((uint32*)(((uint8*)mWorkBuffer) + compressedSize)) = (uint32)checksum;
	compressedSize += sizeof(uint32);

	// Allocate chunk table entry

	assert(mChunkTableSegmentCursor < kChunkTableSegmentEntryCount);
	mChunkTableSegment[mChunkTableSegmentCursor].Offset = mFileSize;
	mChunkTableSegment[mChunkTableSegmentCursor].SizeUncompressed = (uint32)length;
	mChunkTableSegment[mChunkTableSegmentCursor].SizeCompressed = (uint32)compressedSize;
	mChunkTableSegmentCursor++;

	uint64 chunkIndex = mTotalChunkCount++;

	// Write data to LBCF file

	mFile.Write(mWorkBuffer, (uint32)compressedSize);
	mFileSize += compressedSize;

	return chunkIndex;
}

void CLBCFFileWriter::Flush() {
	// Flush final chunk table segment if necessary - overwrites work buffer

	if (mChunkTableSegmentCursor > 0)
		FlushChunkTableSegment();

	// Format descriptor data

	CAlignedBuffer descriptorBufferWrapper(sizeof(SLBCFDescriptorHeader) + sizeof(SLBCFChunkTableIndexEntry) * mChunkTableIndexCursor + mUserDataSize + 16 * 3);
	uint8 *descriptorBuffer = (uint8*)descriptorBufferWrapper.GetBuffer();

	usize headerSize = sizeof(SLBCFDescriptorHeader);
	if (headerSize % 16 != 0)
		headerSize += 16 - (headerSize % 16);

	usize chunkTableIndexSize = sizeof(SLBCFChunkTableIndexEntry) * mChunkTableIndexCursor;
	if (chunkTableIndexSize % 16 != 0)
		chunkTableIndexSize += 16 - (chunkTableIndexSize % 16);

	usize userDataSize = mUserDataSize;
	if (userDataSize % 16 != 0)
		userDataSize += 16 - (userDataSize % 16);

	usize paddedTotalSize = headerSize + chunkTableIndexSize + userDataSize;

	SLBCFDescriptorHeader *descriptorHeader = (SLBCFDescriptorHeader*)descriptorBuffer;
	uint8 *chunkTableIndexBuffer = descriptorBuffer + headerSize;
	uint8 *userDataBuffer = chunkTableIndexBuffer + chunkTableIndexSize;

	if (mChunkTableIndexCursor > 0)
		memcpy(chunkTableIndexBuffer, mChunkTableIndex, sizeof(SLBCFChunkTableIndexEntry) * mChunkTableIndexCursor);

	if (mUserDataSize > 0)
		memcpy(userDataBuffer, mUserData, mUserDataSize);

	descriptorHeader->Signature = SLBCFDescriptorHeader::kSignature;
	descriptorHeader->ChunkTableIndexOffset = headerSize;
	descriptorHeader->ChunkTableIndexEntryCount = mChunkTableIndexCursor;
	descriptorHeader->UserDataOffset = headerSize + chunkTableIndexSize;
	descriptorHeader->UserDataSize = mUserDataSize;

	// Compress descriptor data

	CAlignedBuffer packedBufferWrapper(CDeflate::GetBound(paddedTotalSize, CDeflate::kMaximumCompressionLevel));
	uint8 *packedBuffer = (uint8*)packedBufferWrapper.GetBuffer();

	ulong checksum = CCRC32::CalculateCRC32(descriptorBuffer, paddedTotalSize);
	usize compressedSize;

	if (!CDeflate::Compress(descriptorBuffer, paddedTotalSize, packedBuffer, packedBufferWrapper.GetSize(), compressedSize, CDeflate::kMaximumCompressionLevel))
		gExceptionManager.RaiseException(CException::Generic, "Error compressing chunk data");

	// Prepare file header

	SLBCFFileHeader fileHeader;
	fileHeader.Signature = SLBCFFileHeader::kSignature;
	fileHeader.FileSize = mFileSize + compressedSize;
	fileHeader.DescriptorOffset = mFileSize;
	fileHeader.DescriptorSizeUncompressed = paddedTotalSize;
	fileHeader.DescriptorSizeCompressed = compressedSize;
	fileHeader.DescriptorChecksum = checksum;
	fileHeader.HeaderChecksum = CCRC32::CalculateCRC32(&fileHeader, sizeof(SLBCFFileHeader) - sizeof(uint32));

	// Write data to file

	mFile.Write(packedBuffer, (uint32)compressedSize);
	mFile.SetAbsolutePosition(0);
	mFile.Write(&fileHeader, sizeof(SLBCFFileHeader));
	mFile.SetAbsolutePosition(mFileSize);
}
