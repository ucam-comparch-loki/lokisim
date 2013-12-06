/*
 * Loki memory analysis and simulation tool
 *
 * Copyright (C) 2010-2013 Andreas Koltes. All rights reserved.
 *
 * Loki binary container file management
 *
 * Adapted version for stand-alone SystemC simulator integration
 */

#ifndef LBCFFILE_H_
#define LBCFFILE_H_

#include "LokiTRTLite.h"

/*
 * Loki binary container file format definitions
 */

#pragma pack(push)
#pragma pack(1)

struct SLBCFFileHeader {
public:
	static const uint64 kSignature = ((uint64)('L'))  // Little endian byte order / LSB first
		| (((uint64)('B')) << 8)
		| (((uint64)('C')) << 16)
		| (((uint64)('F')) << 24)
		| (((uint64)('$')) << 32)
		| (((uint64)('1')) << 40)
		| (((uint64)('1')) << 48)
		| (26ULL << 56);

	uint64 Signature;
	uint64 FileSize;
	uint64 DescriptorOffset;
	uint64 DescriptorSizeUncompressed;
	uint64 DescriptorSizeCompressed;
	uint32 DescriptorChecksum;
	uint32 HeaderChecksum;
};

struct SLBCFDescriptorHeader {
public:
	static const uint64 kSignature = ((uint64)('L'))  // Little endian byte order / LSB first
		| (((uint64)('B')) << 8)
		| (((uint64)('C')) << 16)
		| (((uint64)('F')) << 24)
		| (((uint64)('$')) << 32)
		| (((uint64)('D')) << 40)
		| (((uint64)('1')) << 48)
		| (((uint64)('1')) << 56);

	uint64 Signature;
	uint64 ChunkTableIndexOffset;
	uint64 ChunkTableIndexEntryCount;
	uint64 UserDataOffset;
	uint64 UserDataSize;
};

struct SLBCFChunkTableIndexEntry {
public:
	uint64 Offset;
	uint32 EntryCount;
	uint32 SizeStored;
	uint32 Checksum;
};

struct SLBCFChunkTableEntry {
public:
	uint64 Offset;
	uint32 SizeUncompressed;
	uint32 SizeCompressed;  // Includes size of checksum at the end of the compressed data
};

#pragma pack(pop)

/*
 * Loki binary container file writer class
 */

class CLBCFFileWriter {
public:
	static const usize kMaximumChunkSize = 64 * 1024 * 1024;
private:
	static const usize kChunkTableIndexCapacityInitial = 65536;
	static const usize kChunkTableIndexCapacityIncrement = 32768;

	static const usize kChunkTableSegmentEntryCount = 4 * 1024 * 1024;	// 64 MB

	CFile &mFile;
	uint64 mFileSize;

	CDynamicAlignedBuffer mWorkBufferWrapper;
	uint8 *mWorkBuffer;

	CDynamicAlignedBuffer mChunkTableIndexWrapper;
	SLBCFChunkTableIndexEntry *mChunkTableIndex;
	usize mChunkTableIndexCursor;
	usize mChunkTableIndexCapacity;

	CAlignedBuffer mChunkTableSegmentWrapper;
	SLBCFChunkTableEntry *mChunkTableSegment;
	usize mChunkTableSegmentCursor;

	uint64 mTotalChunkCount;

	CDynamicAlignedBuffer mUserDataWrapper;
	void *mUserData;
	usize mUserDataSize;

	void FlushChunkTableSegment();
public:
	CLBCFFileWriter(CFile &lbcfFile);
	~CLBCFFileWriter();

	void SetUserData(const void *buffer, usize length);
	uint64 AppendChunk(const void *buffer, usize length);

	void Flush();

	inline const char* GetFileName() const {
		return mFile.GetFileName();
	}
};

#endif /*LBCFFILE_H_*/
