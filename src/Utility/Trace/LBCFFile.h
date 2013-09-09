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
		| (((uint64)('0')) << 48)
		| (26ULL << 56);

	uint64 Signature;
	uint64 FileSize;
	uint64 DescriptorOffset;
	uint32 DescriptorSizeUncompressed;
	uint32 DescriptorSizeCompressed;
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
		| (((uint64)('H')) << 48)
		| (((uint64)('1')) << 56);

	uint64 Signature;
	uint32 ChunkTableOffset;
	uint32 ChunkTableEntries;
	uint32 UserDataOffset;
	uint32 UserDataSize;
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
	static const usize kChunkTableEntriesIncrement = 1024;

	CFile &mFile;
	uint64 mFileSize;

	CDynamicAlignedBuffer mWorkBufferWrapper;
	uint8 *mWorkBuffer;

	CDynamicAlignedBuffer mChunkTableWrapper;
	SLBCFChunkTableEntry *mChunkTable;
	usize mChunkTableEntries;

	CDynamicAlignedBuffer mUserDataWrapper;
	void *mUserData;
	usize mUserDataSize;
public:
	CLBCFFileWriter(CFile &lbcfFile);
	~CLBCFFileWriter();

	void SetUserData(const void *buffer, usize length);
	ulong AppendChunk(const void *buffer, usize length);

	void Flush();

	inline const char* GetFileName() const			{ return mFile.GetFileName(); }
};

/*
 * Loki binary container file reader class
 */

class CLBCFFileReader {
private:
	CFile &mFile;
	uint64 mFilePosition;

	CDynamicAlignedBuffer mDescriptorWrapper;

	SLBCFDescriptorHeader *mDescriptorHeader;

	SLBCFChunkTableEntry *mChunkTable;
	usize mChunkTableEntries;

	void *mUserData;
	usize mUserDataSize;

	CDynamicAlignedBuffer mChunkBuffer;
public:
	CLBCFFileReader(CFile &lbcfFile);
	~CLBCFFileReader();

	inline usize GetChunkCount() const				{ return mChunkTableEntries; }

	inline usize GetChunkSize(usize index) const {
		if (index >= mChunkTableEntries) {
			cerr << "LBCF file chunk index out of range" << endl;
			exit(1);
		}

		return mChunkTable[index].SizeUncompressed;
	}

	bool ReadChunk(usize index, void *buffer, usize &length);

	inline const void* GetUserData() const			{ return mUserData; }
	inline usize GetUserDataSize() const			{ return mUserDataSize; }

	inline const char* GetFileName() const			{ return mFile.GetFileName(); }
};

#endif /*LBCFFILE_H_*/
