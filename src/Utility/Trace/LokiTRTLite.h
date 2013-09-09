/*
 * Loki tool run-time support library
 *
 * Copyright (C) 2010-2013 Andreas Koltes. All rights reserved.
 *
 * Adapted and reduced version for stand-alone SystemC simulator integration
 */

#ifndef LOKITRTLITE_H_
#define LOKITRTLITE_H_

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

using std::cerr;
using std::endl;

typedef	signed char				sint8;		// Exactly n bits
typedef	unsigned char			uint8;
typedef	signed short			sint16;
typedef	unsigned short			uint16;
typedef	signed int				sint32;
typedef	unsigned int			uint32;
typedef	signed long long		sint64;
typedef	unsigned long long		uint64;

typedef	signed int				sint;		// At least 16 bits
typedef	unsigned int			uint;
typedef	signed long				slong;		// At least 32 bits
typedef	unsigned long			ulong;

typedef	size_t					usize;		// Must be able to represent arbitrary memory offsets

/*
 * Exception handling
 */

class CException {
	friend class CExceptionManager;
public:
	enum EExceptionType {
		Invalid,
		Generic,
		OSGeneric,
		OSMemory,
		OSFileIO,
		OSThreading,
		FileIO,
		FileFormat,
		FileData,
		LogicalFormat,
		LogicalData
	};
public:
	CException() {
		// Nothing
	}

	~CException() {
		// Nothing
	}
};

class CExceptionManager {
public:
	CExceptionManager() {
		// Nothing
	}

	~CExceptionManager() {
		// Nothing
	}

	void RaiseException(CException::EExceptionType exceptionType, const char *errorMessage);
	void RaiseException(CException::EExceptionType exceptionType, const char *errorMessage, const char *fileName);

	void RaiseOSException(CException::EExceptionType exceptionType, const char *errorMessage);
	void RaiseOSException(CException::EExceptionType exceptionType, const char *errorMessage, const char *fileName);
};

extern CExceptionManager gExceptionManager;

/*
 * Memory utility class
 */

class CMemory {
	friend class CAlignedBuffer;
	friend class CDynamicAlignedBuffer;
private:
#ifndef _WIN32
	struct SMemoryDirectory {
	private:
		static const uint kMemoryDirectoryHashTableBits = 8;
		static const usize kMemoryDirectoryHashTableSize = 1UL << kMemoryDirectoryHashTableBits;
		static const usize kMemoryDirectoryHashTableMask = kMemoryDirectoryHashTableSize - 1UL;

		struct SMemoryDirectoryEntry {
		public:
			void *Address;
			usize Size;

			SMemoryDirectoryEntry *PrevEntry;
			SMemoryDirectoryEntry *NextEntry;
		};

		SMemoryDirectoryEntry* mHashTable[kMemoryDirectoryHashTableSize];
	public:
		SMemoryDirectory() {
			memset(mHashTable, 0x00, sizeof(mHashTable));
		}

		inline void Insert(void *address, usize size) {
			usize hash = (((usize)address) >> 12) & kMemoryDirectoryHashTableMask;

			SMemoryDirectoryEntry *entry = new SMemoryDirectoryEntry;
			entry->Address = address;
			entry->Size = size;
			entry->NextEntry = mHashTable[hash];

			mHashTable[hash] = entry;
		}

		inline usize Remove(void *address) {
			usize hash = (((usize)address) >> 12) & kMemoryDirectoryHashTableMask;

			SMemoryDirectoryEntry *prevEntry = NULL;
			SMemoryDirectoryEntry *entry = mHashTable[hash];
			assert(entry != NULL);

			while (entry->Address != address) {
				prevEntry = entry;
				entry = entry->NextEntry;
				assert(entry != NULL);
			}

			if (prevEntry == NULL)
				mHashTable[hash] = entry->NextEntry;
			else
				prevEntry->NextEntry = entry->NextEntry;

			usize result = entry->Size;
			delete entry;
			return result;
		}
	};

	static SMemoryDirectory mMemoryDirectory;
#endif

	CMemory() {
		// Nothing
	}

	~CMemory() {
		// Nothing
	}

	inline static void* AllocateAlignedBufferFast(usize size) {
#ifdef _WIN32
		LPVOID buffer = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		if (buffer == NULL)
			gExceptionManager.RaiseOSException(CException::OSMemory, "Unable to allocate memory");

#ifdef MEMORY_TRACE
		fprintf(stderr, "+F %p %lu\n", buffer, size);
#endif

		return (void*)buffer;
#else
		void *buffer = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		if (buffer == NULL || mprotect(buffer, size, PROT_READ | PROT_WRITE) != 0)
			gExceptionManager.RaiseOSException(CException::OSMemory, "Unable to allocate memory");

		return buffer;
#endif
	}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4100)
#endif

	inline static void FreeAlignedBufferFast(void *buffer, usize size) {
#ifdef _WIN32
#ifdef MEMORY_TRACE
		fprintf(stderr, "-F %p %lu\n", buffer, size);
#endif

		if (VirtualFree(buffer, 0, MEM_RELEASE) == 0)
			gExceptionManager.RaiseOSException(CException::OSMemory, "Unable to deallocate memory");
#else
		if (munmap(buffer, size) != 0)
			gExceptionManager.RaiseOSException(CException::OSMemory, "Unable to deallocate memory");
#endif
	}

	inline static void FreeAlignedBufferSafeFast(void *buffer, usize size) {
#ifdef _WIN32
#ifdef MEMORY_TRACE
		fprintf(stderr, "-FS %p %lu\n", buffer, size);
#endif

		VirtualFree(buffer, 0, MEM_RELEASE);
#else
		munmap(buffer, size);
#endif
	}

#ifdef _WIN32
#pragma warning(pop)
#endif

public:
	static uint64 GetFreePhysicalMemory(void);

#ifdef _WIN32

	inline static void* AllocateAlignedBuffer(usize size) {
		LPVOID buffer = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		if (buffer == NULL)
			gExceptionManager.RaiseOSException(CException::OSMemory, "Unable to allocate memory");

#ifdef MEMORY_TRACE
		fprintf(stderr, "+ %p %lu\n", buffer, size);
#endif

		return (void*)buffer;
	}

	inline static void FreeAlignedBuffer(void *buffer) {
#ifdef MEMORY_TRACE
		fprintf(stderr, "- %p\n", buffer);
#endif

		if (VirtualFree(buffer, 0, MEM_RELEASE) == 0)
			gExceptionManager.RaiseOSException(CException::OSMemory, "Unable to deallocate memory");
	}

	inline static void FreeAlignedBufferSafe(void *buffer) {
#ifdef MEMORY_TRACE
		fprintf(stderr, "-S %p\n", buffer);
#endif

		VirtualFree(buffer, 0, MEM_RELEASE);
	}

#else

	static void* AllocateAlignedBuffer(usize size);
	static void FreeAlignedBuffer(void *buffer);
	static void FreeAlignedBufferSafe(void *buffer);

#endif

	inline static usize RoundToPageBoundary(usize bufferSize) {
		usize slack = bufferSize % 4096UL;
		return slack == 0 ? bufferSize : bufferSize - slack + 4096UL;
	}
};

/*
 * Aligned buffer class
 */

class CAlignedBuffer {
private:
	void *mBuffer;
	usize mSize;
public:
	CAlignedBuffer(usize size) {
		// Buffer gets set to zero bytes automatically
		mBuffer = CMemory::AllocateAlignedBufferFast(size == 0 ? 1UL : size);
		mSize = size;
	}

	~CAlignedBuffer() {
		CMemory::FreeAlignedBufferSafeFast(mBuffer, mSize == 0 ? 1UL : mSize);
	}

	inline void* GetBuffer() {
		return mBuffer;
	}

	inline usize GetSize() const {
		return mSize;
	}
};

/*
 * Dynamic aligned buffer class
 */

class CDynamicAlignedBuffer {
private:
	void *mBuffer;
	usize mSize;
public:
	CDynamicAlignedBuffer() {
		mBuffer = NULL;
		mSize = 0;
	}

	CDynamicAlignedBuffer(usize size) {
		mBuffer = NULL;  // Protect buffer pointer in case of allocation error
		mSize = 0;

		if (size != 0) {
			// Buffer gets set to zero bytes automatically
			mBuffer = CMemory::AllocateAlignedBufferFast(size);
			mSize = size;
		}
	}

	~CDynamicAlignedBuffer() {
		if (mBuffer != NULL)
			CMemory::FreeAlignedBufferSafeFast(mBuffer, mSize);
	}

	inline void Allocate(usize size) {
		if (mBuffer != NULL) {
			CMemory::FreeAlignedBufferFast(mBuffer, mSize);
			mBuffer = NULL;  // Protect buffer pointer in case of allocation error
			mSize = 0;
		}

		if (size != 0) {
			// Buffer gets set to zero bytes automatically
			mBuffer = CMemory::AllocateAlignedBufferFast(size);
			mSize = size;
		}
	}

	inline void Reallocate(usize size) {
		void *oldBuffer = mBuffer;
		usize oldSize = mSize;

		// Protect buffer pointer in case of allocation error
		mBuffer = NULL;
		mSize = 0;

		if (size != 0) {
			// Buffer gets set to zero bytes automatically
			mBuffer = CMemory::AllocateAlignedBufferFast(size);
			mSize = size;

			// Restore buffer contents and release old buffer
			if (oldBuffer != NULL) {
				memcpy(mBuffer, oldBuffer, (oldSize > size) ? size : oldSize);
				CMemory::FreeAlignedBufferFast(oldBuffer, oldSize);
			}
		}
	}

	inline void Release() {
		if (mBuffer != NULL) {
			CMemory::FreeAlignedBufferFast(mBuffer, mSize);
			mBuffer = NULL;
			mSize = 0;
		}
	}

	inline void ExchangeBuffers(CDynamicAlignedBuffer &buffer) {
		void *tempBuffer = mBuffer;
		usize tempSize = mSize;
		mBuffer = buffer.mBuffer;
		mSize = buffer.mSize;
		buffer.mBuffer = tempBuffer;
		buffer.mSize = tempSize;
	}

	inline void* GetBuffer() {
		return mBuffer;
	}

	inline usize GetSize() const {
		return mSize;
	}
};

/*
 * File management class
 */

class CFile {
private:
#ifdef _WIN32
	HANDLE mHandle;
#else
	int mDescriptor;
#endif

	char *mFileName;
	bool mDeleteOnClose;
public:
	enum EAccessMode {
		ReadOnly,
		WriteOnly,
		ReadWrite
	};

	enum EShareMode {
		Exclusive,
		AllowRead,
		AllowWrite,
		AllowAll
	};

	enum ECreationDisposition {
		CreateAlways,
		CreateNew,
		OpenAlways,
		OpenExisting,
		TruncateExisting
	};

	enum ECacheStrategy {
		Mixed,
		RandomAccess,
		SequentialScan
	};

	enum ESeekOrigin {
		Begin,
		Current,
		End
	};

	CFile(const char *fileName, EAccessMode desiredAccess, EShareMode shareMode, ECreationDisposition creationDisposition, ECacheStrategy cacheStrategy, bool enableWriteCaching)
	{
		size_t len = strlen(fileName);
		mFileName = new char[len + 1];
		strcpy(mFileName, fileName);

	#ifdef _WIN32
		DWORD osDesiredAccess = 0;

		if (desiredAccess == ReadOnly || desiredAccess == ReadWrite)
			osDesiredAccess |= GENERIC_READ;
		if (desiredAccess == WriteOnly || desiredAccess == ReadWrite)
			osDesiredAccess |= GENERIC_WRITE;

		DWORD osShareMode = 0;

		if (shareMode == AllowRead || shareMode == AllowAll)
			osShareMode |= FILE_SHARE_READ;
		if (shareMode == AllowWrite || shareMode == AllowAll)
			osShareMode |= FILE_SHARE_WRITE;

		DWORD osCreationDisposition = 0;

		switch (creationDisposition) {
		case CreateAlways:		osCreationDisposition = CREATE_ALWAYS;		break;
		case CreateNew:			osCreationDisposition = CREATE_NEW;			break;
		case OpenAlways:		osCreationDisposition = OPEN_ALWAYS;		break;
		case OpenExisting:		osCreationDisposition = OPEN_EXISTING;		break;
		case TruncateExisting:	osCreationDisposition = TRUNCATE_EXISTING;	break;
		}

		DWORD osCacheStrategy = 0;

		if (cacheStrategy == RandomAccess)
			osCacheStrategy |= FILE_FLAG_RANDOM_ACCESS;
		if (cacheStrategy == SequentialScan)
			osCacheStrategy |= FILE_FLAG_SEQUENTIAL_SCAN;
		if (!enableWriteCaching)
			osCacheStrategy |= FILE_FLAG_WRITE_THROUGH;

		mHandle = CreateFile(
			mFileName,
			osDesiredAccess,
			osShareMode,
			NULL,
			osCreationDisposition,
			osCacheStrategy,
			NULL);

		if (mHandle == INVALID_HANDLE_VALUE)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to open file", fileName);
	#else
		int flags = 0;

		switch (desiredAccess) {
		case ReadOnly:			flags |= O_RDONLY;			break;
		case WriteOnly:			flags |= O_WRONLY;			break;
		case ReadWrite:			flags |= O_RDWR;			break;
		}

	/* BSD only extension / does not work
		switch (shareMode & (ABSTRACT_FILE_SHARE_ALLOW_READ | ABSTRACT_FILE_SHARE_ALLOW_WRITE)) {
		case AllowRead:			flags |= O_SHLOCK;			break;
		case AllowWrite:		flags |= O_EXLOCK;			break;
		case AllowAll:			flags |= O_EXLOCK;			break;
		}
	*/

		switch (creationDisposition) {
		case CreateAlways:		flags |= O_CREAT | O_TRUNC;	break;
		case CreateNew:			flags |= O_CREAT | O_EXCL;	break;
		case OpenAlways:		flags |= O_CREAT;			break;
		case OpenExisting:		flags |= 0;					break;
		case TruncateExisting:	flags |= O_TRUNC;			break;
		}

	/* Not supported on all platforms
		switch (cacheStrategy) {
		case RandomAccess:		flags |= O_RANDOM;			break;
		case SequentialScan:	flags |= O_SEQUENTIAL;		break;
		}
	*/

		if (!enableWriteCaching)
			flags |= O_FSYNC;

		if ((mDescriptor = open64(mFileName, flags, 6 << 6)) == -1)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to open file", fileName);
	#endif

		mDeleteOnClose = false;
	}

	~CFile() {
#ifdef _WIN32
		CloseHandle(mHandle);

		if (mDeleteOnClose)
			DeleteFile(mFileName);  // Do not throw an exception inside the destructor
#else
		close(mDescriptor);

		if (mDeleteOnClose)
			unlink(mFileName);
#endif

		delete[] mFileName;
	}

	inline bool GetDeleteOnClose() const {
		return mDeleteOnClose;
	}

	inline void SetDeleteOnClose(bool deleteOnClose) {
		mDeleteOnClose = deleteOnClose;
	}

	inline sint64 GetSize() {
#ifdef _WIN32
		LARGE_INTEGER osFileSize;

		if (GetFileSizeEx(mHandle, &osFileSize) == 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file size", mFileName);

		return osFileSize.QuadPart;
#else
		struct stat64 buf;

		if (fstat64(mDescriptor, &buf) != 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file size", mFileName);

		return (uint64)buf.st_size;
#endif
	}

	void SetSize(sint64 size);

	inline sint64 GetPosition() {
#ifdef _WIN32
		LARGE_INTEGER osZero;
		LARGE_INTEGER osFilePosition;

		osZero.QuadPart = 0;

		if (SetFilePointerEx(mHandle, osZero, &osFilePosition, FILE_CURRENT) == 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file position", mFileName);

		return osFilePosition.QuadPart;
#else
		off64_t filePosition = lseek64(mDescriptor, 0, SEEK_CUR);

		if (filePosition < 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file position", mFileName);

		return (sint64)filePosition;
#endif
	}

	inline sint64 SetPosition(sint64 offset, ESeekOrigin origin) {
#ifdef _WIN32
		LARGE_INTEGER osFilePosition;
		osFilePosition.QuadPart = offset;

		DWORD osMoveMethod = 0;

		switch (origin) {
		case Begin:		osMoveMethod = FILE_BEGIN;		break;
		case Current:	osMoveMethod = FILE_CURRENT;	break;
		case End:		osMoveMethod = FILE_END;		break;
		}

		LARGE_INTEGER osFileNewPosition;

		if (SetFilePointerEx(mHandle, osFilePosition, &osFileNewPosition, osMoveMethod) == 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to set file position", mFileName);

		return osFileNewPosition.QuadPart;
#else
		off64_t effectiveOffset = offset;
		int effectiveOrigin;

		switch (origin) {
		case Begin:		effectiveOrigin = SEEK_SET;		break;
		case Current:	effectiveOrigin = SEEK_CUR;		break;
		case End:		effectiveOrigin = SEEK_END;		break;
		}

		off64_t filePosition = lseek64(mDescriptor, effectiveOffset, effectiveOrigin);

		if (filePosition < 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file position", mFileName);

		return (sint64)filePosition;
#endif
	}

	inline void SetAbsolutePosition(uint64 offset) {
#ifdef _WIN32
		LARGE_INTEGER osFilePosition;
		LARGE_INTEGER osFileNewPosition;
		osFilePosition.QuadPart = offset;

		if (SetFilePointerEx(mHandle, osFilePosition, &osFileNewPosition, FILE_BEGIN) == 0 || osFilePosition.QuadPart != osFileNewPosition.QuadPart)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to set file position", mFileName);
#else
		off64_t effectiveOffset = offset;
		off64_t filePosition = lseek64(mDescriptor, effectiveOffset, SEEK_SET);

		if ((uint64)filePosition != offset)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file position", mFileName);
#endif
	}

	inline bool EndOfFile() {
#ifdef _WIN32
		LARGE_INTEGER osZero;
		LARGE_INTEGER osFilePosition;

		osZero.QuadPart = 0;

		if (SetFilePointerEx(mHandle, osZero, &osFilePosition, FILE_CURRENT) == 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file position", mFileName);

		LARGE_INTEGER osFileSize;

		if (GetFileSizeEx(mHandle, &osFileSize) == 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file size", mFileName);

		return osFilePosition.QuadPart == osFileSize.QuadPart;
#else
		off64_t filePosition;
		off64_t fileSize;
		struct stat64 buf;

		if ((filePosition = lseek64(mDescriptor, 0, SEEK_CUR)) < 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file position", mFileName);

		if (fstat64(mDescriptor, &buf) != 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve file size", mFileName);

		fileSize = buf.st_size;

		return filePosition == fileSize;
#endif
	}

	inline void Read(void *buffer, uint32 count) {
		assert(buffer != NULL);

#ifdef _WIN32
		DWORD osBytesRead;

		if (ReadFile(mHandle, buffer, count, &osBytesRead, NULL) == 0 || osBytesRead != count)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to read file data", mFileName);
#else
		if (read(mDescriptor, buffer, count) != count)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to read file data", mFileName);
#endif
	}

	inline void Write(const void *buffer, uint32 count) {
		assert(buffer != NULL);

#ifdef _WIN32
		DWORD osBytesWritten;

		if (WriteFile(mHandle, buffer, count, &osBytesWritten, NULL) == 0 || osBytesWritten != count)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to write file data", mFileName);
#else
		if (write(mDescriptor, buffer, count) != count)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to write file data", mFileName);
#endif
	}

	inline uint32 TryRead(void *buffer, uint32 count) {
		assert(buffer != NULL);

#ifdef _WIN32
		DWORD osBytesRead;

		if (ReadFile(mHandle, buffer, count, &osBytesRead, NULL) == 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to read file data", mFileName);

		return osBytesRead;
#else
		ssize_t bytesRead = read(mDescriptor, buffer, count);

		if (bytesRead < 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to read file data", mFileName);

		return (uint32)bytesRead;
#endif
	}

	inline uint32 TryWrite(const void *buffer, uint32 count) {
		assert(buffer != NULL);

#ifdef _WIN32
		DWORD osBytesWritten;

		if (WriteFile(mHandle, buffer, count, &osBytesWritten, NULL) == 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to write file data", mFileName);

		return osBytesWritten;
#else
		ssize_t bytesWritten = write(mDescriptor, buffer, count);

		if (bytesWritten < 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to write file data", mFileName);

		return (uint32)bytesWritten;
#endif
	}

	inline void Flush() {
#ifdef _WIN32
		if (FlushFileBuffers(mHandle) == 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to flush file buffers", mFileName);
#else
		fsync(mDescriptor);
#endif
	}

	inline const char* GetFileName() const {
		return mFileName;
	}

	inline uint64 GetVolumeIdentifier() {
#ifdef _WIN32
		BY_HANDLE_FILE_INFORMATION osFileInfo;

		if (GetFileInformationByHandle(mHandle, &osFileInfo) == 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve volume identifier", mFileName);

		return osFileInfo.dwVolumeSerialNumber;
#else
		struct stat64 buf;

		if (fstat64(mDescriptor, &buf) != 0)
			gExceptionManager.RaiseOSException(CException::OSFileIO, "Unable to retrieve volume identifier", mFileName);

		return (uint64)buf.st_dev;
#endif
	}

	static inline bool Exists(const char *fileName) {
#ifdef _WIN32
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind;

		hFind = FindFirstFile(fileName, &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE) {
			return false;
		} else {
			FindClose(hFind);
			return true;
		}
#else
		return access(fileName, F_OK) == 0;
#endif
	}
};

/*
 * CRC32 checksum computation
 */

class CCRC32 {
private:
	ulong mCRC;
public:
	CCRC32();
	~CCRC32();

	void Update(const void *buffer, usize length);

	inline ulong GetCRC() {
		return mCRC;
	}

	static ulong CalculateCRC32(const void *buffer, usize length);
};

/*
 * Deflate compression / decompression
 */

class CDeflate {
private:
	CDeflate() {
		// Nothing
	}

	~CDeflate() {
		// Nothing
	}
public:
	static const uint kMinimumCompressionLevel = 1;
	static const uint kMaximumCompressionLevel = 9;

	static bool Compress(const void *inBuffer, usize inLength, void *outBuffer, usize outLength, usize &compressedLength, uint compressionLevel);
	static bool Decompress(const void *inBuffer, usize inLength, void *outBuffer, usize outLength);

	static usize GetBound(usize inLength, uint compressionLevel);
};

#endif /* LOKITRTLITE_H_ */
