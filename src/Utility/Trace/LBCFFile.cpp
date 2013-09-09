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

CLBCFFileWriter::CLBCFFileWriter(CFile &lbcfFile) :
	mFile(lbcfFile),
	mFileSize(sizeof(SLBCFFileHeader)),
	mChunkTableWrapper(),
	mChunkTable(NULL),
	mChunkTableEntries(0),
	mUserDataWrapper(),
	mUserData(NULL),
	mUserDataSize(0)
{
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

ulong CLBCFFileWriter::AppendChunk(const void *buffer, usize length) {
	assert(buffer != NULL);
	assert(length <= kMaximumChunkSize);

	// Allocate work buffer if necessary

	usize minWorkBufferSize = CDeflate::GetBound(length, CDeflate::kMaximumCompressionLevel) + sizeof(uint32);
	if (mWorkBufferWrapper.GetSize() < minWorkBufferSize) {
		mWorkBufferWrapper.Allocate(minWorkBufferSize);
		mWorkBuffer = (uint8*)mWorkBufferWrapper.GetBuffer();
	}

	// Compress chunk data

	ulong checksum = CCRC32::CalculateCRC32(buffer, length);
	usize compressedSize;

	if (!CDeflate::Compress(buffer, length, mWorkBuffer, minWorkBufferSize, compressedSize, CDeflate::kMaximumCompressionLevel))
		gExceptionManager.RaiseException(CException::Generic, "Error compressing chunk data");

	*((uint32*)(((uint8*)mWorkBuffer) + compressedSize)) = (uint32)checksum;
	compressedSize += sizeof(uint32);

	// Allocate chunk table entry

	if ((mChunkTableEntries + 1) * sizeof(SLBCFChunkTableEntry) > mChunkTableWrapper.GetSize()) {
		mChunkTableWrapper.Reallocate(mChunkTableWrapper.GetSize() + sizeof(SLBCFChunkTableEntry) * kChunkTableEntriesIncrement);
		mChunkTable = (SLBCFChunkTableEntry*)mChunkTableWrapper.GetBuffer();
	}

	mChunkTable[mChunkTableEntries].Offset = mFileSize;
	mChunkTable[mChunkTableEntries].SizeUncompressed = (uint32)length;
	mChunkTable[mChunkTableEntries].SizeCompressed = (uint32)compressedSize;
	usize chunkIndex = mChunkTableEntries++;

	// Write data to LBCF file

	mFile.Write(mWorkBuffer, (uint32)compressedSize);
	mFileSize += compressedSize;

	return (ulong)chunkIndex;
}

void CLBCFFileWriter::Flush() {
	// Output jobs are assumed to have finished

	// Format descriptor data

	CAlignedBuffer descriptorBufferWrapper(sizeof(SLBCFDescriptorHeader) + sizeof(SLBCFChunkTableEntry) * mChunkTableEntries + mUserDataSize + 16 * 3);
	uint8 *descriptorBuffer = (uint8*)descriptorBufferWrapper.GetBuffer();

	usize headerSize = sizeof(SLBCFDescriptorHeader);
	if (headerSize % 16 != 0)
		headerSize += 16 - (headerSize % 16);

	usize chunkTableSize = sizeof(SLBCFChunkTableEntry) * mChunkTableEntries;
	if (chunkTableSize % 16 != 0)
		chunkTableSize += 16 - (chunkTableSize % 16);

	usize userDataSize = mUserDataSize;
	if (userDataSize % 16 != 0)
		userDataSize += 16 - (userDataSize % 16);

	usize paddedTotalSize = headerSize + chunkTableSize + userDataSize;

	SLBCFDescriptorHeader *descriptorHeader = (SLBCFDescriptorHeader*)descriptorBuffer;
	uint8 *chunkTableBuffer = descriptorBuffer + headerSize;
	uint8 *userDataBuffer = chunkTableBuffer + chunkTableSize;

	if (mChunkTableEntries > 0)
		memcpy(chunkTableBuffer, mChunkTable, sizeof(SLBCFChunkTableEntry) * mChunkTableEntries);

	if (mUserDataSize > 0)
		memcpy(userDataBuffer, mUserData, mUserDataSize);

	descriptorHeader->Signature = SLBCFDescriptorHeader::kSignature;
	descriptorHeader->ChunkTableOffset = (uint32)headerSize;
	descriptorHeader->ChunkTableEntries = (uint32)mChunkTableEntries;
	descriptorHeader->UserDataOffset = (uint32)(headerSize + chunkTableSize);
	descriptorHeader->UserDataSize = (uint32)mUserDataSize;

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
	fileHeader.DescriptorSizeUncompressed = (uint32)paddedTotalSize;
	fileHeader.DescriptorSizeCompressed = (uint32)compressedSize;
	fileHeader.DescriptorChecksum = checksum;
	fileHeader.HeaderChecksum = CCRC32::CalculateCRC32(&fileHeader, sizeof(SLBCFFileHeader) - sizeof(uint32));

	// Write data to file

	mFile.Write(packedBuffer, (uint32)compressedSize);
	mFile.SetAbsolutePosition(0);
	mFile.Write(&fileHeader, sizeof(SLBCFFileHeader));
	mFile.SetAbsolutePosition(mFileSize);
}

/*
 * Loki binary container file reader class
 */

CLBCFFileReader::CLBCFFileReader(CFile &lbcfFile) :
	mFile(lbcfFile),
	mFilePosition(0),
	mDescriptorWrapper(),
	mDescriptorHeader(NULL),
	mChunkTable(NULL),
	mChunkTableEntries(0),
	mUserData(NULL),
	mUserDataSize(0),
	mChunkBuffer()
{
	// Read and check file header

	mFile.SetAbsolutePosition(0);
	uint64 fileSize = mFile.GetSize();

	if (fileSize < sizeof(SLBCFFileHeader))
		gExceptionManager.RaiseException(CException::FileFormat, "Invalid LBCF file format", mFile.GetFileName());

	SLBCFFileHeader header;
	mFile.Read(&header, sizeof(header));

	if (header.Signature != SLBCFFileHeader::kSignature)
		gExceptionManager.RaiseException(CException::FileFormat, "Invalid LBCF file format", mFile.GetFileName());

	if (header.FileSize != fileSize)
		gExceptionManager.RaiseException(CException::FileFormat, "Unexpected end of LBCF file", mFile.GetFileName());

	ulong checksum = CCRC32::CalculateCRC32(&header, sizeof(SLBCFFileHeader) - sizeof(uint32));
	if (header.HeaderChecksum != checksum ||
		header.DescriptorOffset > fileSize ||
		header.DescriptorOffset + header.DescriptorSizeCompressed > fileSize ||
		header.DescriptorSizeUncompressed < sizeof(SLBCFDescriptorHeader))
		gExceptionManager.RaiseException(CException::FileFormat, "LBCF file header failed integrity check", mFile.GetFileName());

	// Read and check descriptor

	mChunkBuffer.Allocate(header.DescriptorSizeCompressed);
	void *buffer = mChunkBuffer.GetBuffer();

	mFile.SetAbsolutePosition(header.DescriptorOffset);
	mFile.Read(buffer, header.DescriptorSizeCompressed);
	mFilePosition = header.DescriptorOffset + header.DescriptorSizeCompressed;

	mDescriptorWrapper.Allocate(header.DescriptorSizeUncompressed);
	uint8 *descriptorBuffer = (uint8*)mDescriptorWrapper.GetBuffer();

	if (!CDeflate::Decompress(buffer, header.DescriptorSizeCompressed, descriptorBuffer, header.DescriptorSizeUncompressed))
		gExceptionManager.RaiseException(CException::FileData, "Error decompressing LBCF file descriptor", mFile.GetFileName());

	ulong descriptorChecksum = CCRC32::CalculateCRC32(descriptorBuffer, header.DescriptorSizeUncompressed);
	if (descriptorChecksum != header.DescriptorChecksum)
		gExceptionManager.RaiseException(CException::LogicalData, "LBCF file descriptor failed integrity check", mFile.GetFileName());

	mDescriptorHeader = (SLBCFDescriptorHeader*)descriptorBuffer;

	mChunkTable = (SLBCFChunkTableEntry*)(descriptorBuffer + mDescriptorHeader->ChunkTableOffset);
	mChunkTableEntries = mDescriptorHeader->ChunkTableEntries;

	mUserData = descriptorBuffer + mDescriptorHeader->UserDataOffset;
	mUserDataSize = mDescriptorHeader->UserDataSize;
}

CLBCFFileReader::~CLBCFFileReader() {
	// Nothing
}

bool CLBCFFileReader::ReadChunk(usize index, void *buffer, usize &length) {
	assert(buffer != NULL);

	// Check arguments

	if (index >= mChunkTableEntries)
		gExceptionManager.RaiseException(CException::FileIO, "LBCF file chunk index out of range", mFile.GetFileName());

	uint64 fileOffset = mChunkTable[index].Offset;
	usize sizeUncompressed = mChunkTable[index].SizeUncompressed;
	usize sizeCompressed = mChunkTable[index].SizeCompressed;

	if (length < sizeUncompressed) {
		length = sizeUncompressed;
		return false;
	}

	// Read and check chunk data

	if (mChunkBuffer.GetSize() < sizeCompressed)
		mChunkBuffer.Allocate(sizeCompressed);
	uint8 *chunkBuffer = (uint8*)mChunkBuffer.GetBuffer();

	if (mFilePosition != fileOffset)
		mFile.SetAbsolutePosition(fileOffset);
	mFile.Read(chunkBuffer, (uint32)sizeCompressed);
	mFilePosition = fileOffset + sizeCompressed;

	sizeCompressed -= sizeof(uint32);
	if (!CDeflate::Decompress(chunkBuffer, sizeCompressed, buffer, sizeUncompressed))
		gExceptionManager.RaiseException(CException::FileData, "Error decompressing LBCF file chunk", mFile.GetFileName());

	ulong checksum = CCRC32::CalculateCRC32(buffer, sizeUncompressed);
	if (checksum != *((uint32*)(chunkBuffer + sizeCompressed)))
		gExceptionManager.RaiseException(CException::LogicalData, "LBCF file chunk failed integrity check", mFile.GetFileName());

	length = sizeUncompressed;
	return true;
}
