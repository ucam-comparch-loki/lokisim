/*
 * Loki tool run-time support library
 *
 * Copyright (C) 2010-2013 Andreas Koltes. All rights reserved.
 *
 * Adapted and reduced version for stand-alone SystemC simulator integration
 */

#include "LokiTRTLite.h"

#include <zlib.h>

/*
 * Exception handling
 */

void CExceptionManager::RaiseException(CException::EExceptionType exceptionType, const char *errorMessage) {
	cerr << "ERROR: " << errorMessage << endl;
	exit(EXIT_FAILURE);
}

void CExceptionManager::RaiseException(CException::EExceptionType exceptionType, const char *errorMessage, const char *fileName) {
	cerr << "ERROR: " << errorMessage << " (" << fileName << ")" << endl;
	exit(EXIT_FAILURE);
}

void CExceptionManager::RaiseOSException(CException::EExceptionType exceptionType, const char *errorMessage) {
	const char fallbackMessage[] = "Unknown operating system error code";

	int osErrorCode = errno;
	const char *message = strerror(osErrorCode);
	if (message == NULL)
		message = fallbackMessage;

	cerr << "ERROR: " << errorMessage << " (" << message << ")" << endl;
	exit(EXIT_FAILURE);
}

void CExceptionManager::RaiseOSException(CException::EExceptionType exceptionType, const char *errorMessage, const char *fileName) {
	const char fallbackMessage[] = "Unknown operating system error code";

	int osErrorCode = errno;
	const char *message = strerror(osErrorCode);
	if (message == NULL)
		message = fallbackMessage;

	cerr << "ERROR: " << errorMessage << " (" << message << ") (" << fileName << ")" << endl;
	exit(EXIT_FAILURE);
}

CExceptionManager gExceptionManager;

/*
 * Memory utility class
 */

#ifndef _WIN32
CMemory::SMemoryDirectory CMemory::mMemoryDirectory;
#endif

uint64 CMemory::GetFreePhysicalMemory(void) {
#ifdef _WIN32
	MEMORYSTATUSEX memoryStatus;
	memoryStatus.dwLength = sizeof(memoryStatus);

	if (GlobalMemoryStatusEx(&memoryStatus) == 0)
		gExceptionManager.RaiseOSException(CException::OSGeneric, "Unable to retrieve free amount of physical memory");

	return memoryStatus.ullAvailPhys;
#else
	uint64 pageSize = sysconf(_SC_PAGESIZE);
	uint64 availablePages = sysconf(_SC_AVPHYS_PAGES);
	return pageSize * availablePages;
#endif
}

#ifndef _WIN32

void* CMemory::AllocateAlignedBuffer(usize size) {
	void *buffer = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (buffer == NULL || mprotect(buffer, size, PROT_READ | PROT_WRITE) != 0)
		gExceptionManager.RaiseOSException(CException::OSMemory, "Unable to allocate memory");

	mMemoryDirectory.Insert(buffer, size);

	return buffer;
}

void CMemory::FreeAlignedBuffer(void *buffer) {
	if (munmap(buffer, mMemoryDirectory.Remove(buffer)) != 0)
		gExceptionManager.RaiseOSException(CException::OSMemory, "Unable to deallocate memory");
}

void CMemory::FreeAlignedBufferSafe(void *buffer) {
	munmap(buffer, mMemoryDirectory.Remove(buffer));
}

#endif

/*
 * File management class
 */

void CFile::SetSize(sint64 size) {
	assert(size >= 0);

#ifdef _WIN32
	LARGE_INTEGER osZero;
	LARGE_INTEGER osFilePosition;
	LARGE_INTEGER osFileEndPosition;
	LARGE_INTEGER osFileSize;
	LARGE_INTEGER osFileNewSize;

	osZero.QuadPart = 0;
	osFileSize.QuadPart = size;

	if (SetFilePointerEx(mHandle, osZero, &osFilePosition, FILE_CURRENT) == 0)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file position", mFileName);

	if (SetFilePointerEx(mHandle, osFileSize, &osFileEndPosition, FILE_BEGIN) == 0 || osFileEndPosition.QuadPart != size)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to set file position", mFileName);

	if (SetEndOfFile(mHandle) == 0)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to set file size", mFileName);

	if (GetFileSizeEx(mHandle, &osFileNewSize) == 0)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file size", mFileName);

	if (osFileNewSize.QuadPart != size)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to set file size", mFileName);

	if (SetFilePointerEx(mHandle, (osFilePosition.QuadPart > osFileNewSize.QuadPart) ? osFileNewSize : osFilePosition, NULL, FILE_BEGIN) == 0)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to set file position", mFileName);
#else
	off64_t filePosition;
	off64_t fileSize;
	struct stat64 buf;

	if ((filePosition = lseek64(mDescriptor, 0, SEEK_CUR)) < 0)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to set file position", mFileName);

	if (fstat64(mDescriptor, &buf) != 0)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file size", mFileName);

	fileSize = buf.st_size;

	if (ftruncate64(mDescriptor, size) != 0)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to set file size", mFileName);

	if (fstat64(mDescriptor, &buf) != 0)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file size", mFileName);

	if (buf.st_size != size)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to set file size", mFileName);

	fileSize = buf.st_size;

	if ((filePosition = lseek64(mDescriptor, (filePosition > fileSize) ? 0 : filePosition, SEEK_SET)) < 0)
		gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to set file position", mFileName);
#endif
}

/*
 * CRC32 checksum computation
 */

CCRC32::CCRC32() {
	mCRC = crc32(0L, Z_NULL, 0);
}

CCRC32::~CCRC32() {
	// Nothing
}

void CCRC32::Update(const void *buffer, usize length) {
	mCRC = crc32(mCRC, (const Bytef*)buffer, (uInt)length);
}

ulong CCRC32::CalculateCRC32(const void *buffer, usize length) {
	return crc32(crc32(0L, Z_NULL, 0), (const Bytef*)buffer, (uInt)length);
}

/*
 * Deflate compression / decompression
 */

bool CDeflate::Compress(const void *inBuffer, usize inLength, void *outBuffer, usize outLength, usize &compressedLength, uint compressionLevel) {
	z_stream stream;
    int err;

    stream.next_in = (Bytef*)inBuffer;
	stream.avail_in = (uInt)inLength;
    stream.next_out = (Bytef*)outBuffer;
	stream.avail_out = (uInt)outLength;
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;

	if (compressionLevel < Z_BEST_SPEED)
		compressionLevel = Z_BEST_SPEED;
	else if (compressionLevel > Z_BEST_COMPRESSION)
		compressionLevel = Z_BEST_COMPRESSION;

	err = deflateInit2(&stream, compressionLevel, Z_DEFLATED, -15, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (err != Z_OK)
		return false;

	err = deflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		inflateEnd(&stream);
		return false;
	}

	compressedLength = outLength - stream.avail_out;

    err = deflateEnd(&stream);
	return err == Z_OK;
}

bool CDeflate::Decompress(const void *inBuffer, usize inLength, void *outBuffer, usize outLength) {
	z_stream stream;
    int err;

    stream.next_in = (Bytef*)inBuffer;
	stream.avail_in = (uInt)inLength;
    stream.next_out = (Bytef*)outBuffer;
	stream.avail_out = (uInt)outLength;
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;

	err = inflateInit2(&stream, -15);
	if (err != Z_OK)
		return false;

    err = inflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END || stream.total_out != outLength) {
		inflateEnd(&stream);
		return false;
	}

    err = inflateEnd(&stream);
	return err == Z_OK;
}

usize CDeflate::GetBound(usize inLength, uint compressionLevel) {
	z_stream stream;
    int err;

    stream.next_in = (Bytef*)NULL;
	stream.avail_in = (uInt)0;
    stream.next_out = (Bytef*)NULL;
	stream.avail_out = (uInt)0;
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;

	if (compressionLevel < Z_BEST_SPEED)
		compressionLevel = Z_BEST_SPEED;
	else if (compressionLevel > Z_BEST_COMPRESSION)
		compressionLevel = Z_BEST_COMPRESSION;

	err = deflateInit2(&stream, compressionLevel, Z_DEFLATED, -15, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (err != Z_OK)
		return inLength + inLength / 4 + 4096;  // Conservative bound in case of an error

	usize bound = deflateBound(&stream, (uLong)inLength);

    deflateEnd(&stream);  // Ignore errors - bound already determined

    return bound;
}
