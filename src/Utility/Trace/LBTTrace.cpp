/*
 * LBTTrace.cpp
 *
 *  Created on: 9 Sep 2013
 *      Author: afjk2
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "../Parameters.h"
#include "LBTFile.h"
#include "LBTTrace.h"

namespace LBTTrace {

static CFile *traceFile = NULL;
static CLBTFileWriter *traceWriter = NULL;

static uint64 traceClockCycle = 0;

struct LBTTraceInstruction {
public:
	unsigned long long ISID;
	unsigned long long ClockCycle;

	unsigned long InstructionAddress;
	LBTOperationType OperationType;
	bool Executed;
	bool EndOfPacket;

	unsigned long MemoryAddress;
	bool MemoryAddressSet;

	unsigned long MemoryData;
	bool MemoryDataSet;

	unsigned long InputChannel1;
	unsigned long InputChannel2;
	bool UsesInputChannel1;
	bool UsesInputChannel2;

	unsigned long SystemCallNumber;
	unsigned long *RegisterValues;
	unsigned long RegisterCount;
	unsigned char *ExtraData;
	unsigned long ExtraDataLength;
	bool SystemCallInformationSet;
};

static const unsigned long kTraceInstructionBufferSize = 64;

static LBTTraceInstruction traceInstructionBuffer[kTraceInstructionBufferSize];
static unsigned long traceInstructionCursor = 0;
static unsigned long long traceNextISID = 1;
static bool traceExitCallReached = false;
static bool traceInstructionBufferFlushed = false;

static uint32 *traceMemoryImage = NULL;

// Internal helper routine

void flushBufferedInstruction() {
	if (traceInstructionBuffer[traceInstructionCursor].ISID != 0 && !traceExitCallReached) {
		// Write pending instruction

		uint32 modData;
		uint shift;
		uint32 mask;

		switch (traceInstructionBuffer[traceInstructionCursor].OperationType) {
		case NOP:
			traceWriter->AddBasicOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeNOP,
				0,
				false,
				0,
				false,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case ALU1:
			traceWriter->AddBasicOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeALU1,
				traceInstructionBuffer[traceInstructionCursor].InputChannel1,
				traceInstructionBuffer[traceInstructionCursor].UsesInputChannel1,
				traceInstructionBuffer[traceInstructionCursor].InputChannel2,
				traceInstructionBuffer[traceInstructionCursor].UsesInputChannel2,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case ALU2:
			traceWriter->AddBasicOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeALU2,
				traceInstructionBuffer[traceInstructionCursor].InputChannel1,
				traceInstructionBuffer[traceInstructionCursor].UsesInputChannel1,
				traceInstructionBuffer[traceInstructionCursor].InputChannel2,
				traceInstructionBuffer[traceInstructionCursor].UsesInputChannel2,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case Fetch:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);

			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeFetch,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				0,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case ScratchpadRead:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);

			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeScratchpadRead,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				0,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case ScratchpadWrite:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryDataSet);

			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeScratchpadWrite,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].MemoryData,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case LoadImmediate:
			traceWriter->AddBasicOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeLoadImmediate,
				0,
				false,
				0,
				false,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case SystemCall:
			assert(traceInstructionBuffer[traceInstructionCursor].SystemCallInformationSet);

			if (traceInstructionBuffer[traceInstructionCursor].SystemCallNumber == 0x1)
				traceExitCallReached = true;

			traceWriter->AddSystemCall(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				traceInstructionBuffer[traceInstructionCursor].SystemCallNumber,
				traceInstructionBuffer[traceInstructionCursor].RegisterValues,
				traceInstructionBuffer[traceInstructionCursor].RegisterCount,
				traceInstructionBuffer[traceInstructionCursor].ExtraData,
				traceInstructionBuffer[traceInstructionCursor].ExtraDataLength,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			if (traceInstructionBuffer[traceInstructionCursor].RegisterCount > 0)
				delete[] traceInstructionBuffer[traceInstructionCursor].RegisterValues;

			if (traceInstructionBuffer[traceInstructionCursor].ExtraDataLength > 0)
				delete[] traceInstructionBuffer[traceInstructionCursor].ExtraData;

			break;
		case Control:
			traceWriter->AddBasicOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeControl,
				traceInstructionBuffer[traceInstructionCursor].InputChannel1,
				traceInstructionBuffer[traceInstructionCursor].UsesInputChannel1,
				traceInstructionBuffer[traceInstructionCursor].InputChannel2,
				traceInstructionBuffer[traceInstructionCursor].UsesInputChannel2,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case LoadWord:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddress % 4 == 0);

			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeLoadWord,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceMemoryImage[traceInstructionBuffer[traceInstructionCursor].MemoryAddress / 4],
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case LoadHalfWord:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddress % 2 == 0);

			shift = (traceInstructionBuffer[traceInstructionCursor].MemoryAddress & 0x3UL) * 8;

			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeLoadHalfWord,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				(traceMemoryImage[traceInstructionBuffer[traceInstructionCursor].MemoryAddress / 4] >> shift) & 0xFFFFUL,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case LoadByte:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);

			shift = (traceInstructionBuffer[traceInstructionCursor].MemoryAddress & 0x3UL) * 8;

			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeLoadByte,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				(traceMemoryImage[traceInstructionBuffer[traceInstructionCursor].MemoryAddress / 4] >> shift) & 0xFFUL,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case StoreWord:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryDataSet);
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddress % 4 == 0);

			traceMemoryImage[traceInstructionBuffer[traceInstructionCursor].MemoryAddress / 4] = traceInstructionBuffer[traceInstructionCursor].MemoryData;

			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeStoreWord,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].MemoryData,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		case StoreHalfWord:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryDataSet);
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddress % 2 == 0);

			modData = traceMemoryImage[traceInstructionBuffer[traceInstructionCursor].MemoryAddress / 4];
			shift = (traceInstructionBuffer[traceInstructionCursor].MemoryAddress & 0x3UL) * 8;
			mask = 0xFFFFUL << shift;
			modData &= ~mask;
			modData |= (traceInstructionBuffer[traceInstructionCursor].MemoryData & 0xFFFFUL) << shift;
			traceMemoryImage[traceInstructionBuffer[traceInstructionCursor].MemoryAddress / 4] = modData;

			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeStoreHalfWord,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].MemoryData,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);
			break;
		case StoreByte:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryDataSet);

			modData = traceMemoryImage[traceInstructionBuffer[traceInstructionCursor].MemoryAddress / 4];
			shift = (traceInstructionBuffer[traceInstructionCursor].MemoryAddress & 0x3UL) * 8;
			mask = 0xFFUL << shift;
			modData &= ~mask;
			modData |= (traceInstructionBuffer[traceInstructionCursor].MemoryData & 0xFFUL) << shift;
			traceMemoryImage[traceInstructionBuffer[traceInstructionCursor].MemoryAddress / 4] = modData;

			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkExtendedRecord::kOperationTypeStoreByte,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].MemoryData,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);

			break;
		}
	}
}

// Initialize trace writer

void start(const std::string& filename) {
	traceFile = new CFile(filename.c_str(), CFile::ReadWrite, CFile::AllowRead, CFile::CreateAlways, CFile::SequentialScan, true);
	traceWriter = new CLBTFileWriter(*traceFile);

	memset(traceInstructionBuffer, 0x00, sizeof(traceInstructionBuffer));
	traceInstructionCursor = 0;
	traceNextISID = 1;
	traceExitCallReached = false;
	traceInstructionBufferFlushed = false;
}

// Update clock cycle

void setClockCycle(unsigned long long cycleNumber) {
	traceClockCycle = cycleNumber;
}

// Log activity

unsigned long long logDecodedInstruction(unsigned long instructionAddress, LBTOperationType operationType, bool endOfPacket) {
	assert((instructionAddress & 0x3) == 0);

	//cerr << "- " << instructionAddress << endl;

	if (traceWriter != NULL) {
		flushBufferedInstruction();

		unsigned long long isid = traceNextISID++;

		traceInstructionBuffer[traceInstructionCursor].ISID = isid;
		traceInstructionBuffer[traceInstructionCursor].ClockCycle = traceClockCycle;
		traceInstructionBuffer[traceInstructionCursor].InstructionAddress = instructionAddress;
		traceInstructionBuffer[traceInstructionCursor].OperationType = operationType;
		traceInstructionBuffer[traceInstructionCursor].EndOfPacket = endOfPacket;
		traceInstructionBuffer[traceInstructionCursor].Executed = true;
		traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet = false;
		traceInstructionBuffer[traceInstructionCursor].UsesInputChannel1 = false;
		traceInstructionBuffer[traceInstructionCursor].UsesInputChannel2 = false;
		traceInstructionBuffer[traceInstructionCursor].SystemCallInformationSet = false;

		traceInstructionCursor++;
		if (traceInstructionCursor == kTraceInstructionBufferSize)
			traceInstructionCursor = 0;

		return isid;
	} else {
		return 0;
	}
}

void setInstructionMemoryAddress(unsigned long long isid, unsigned long address) {
	if (traceWriter != NULL) {
		for (unsigned long i = 0; i < kTraceInstructionBufferSize; i++) {
			if (traceInstructionBuffer[i].ISID == isid) {
				if (!((traceInstructionBuffer[i].OperationType == LoadWord && (address & 0x3) == 0 && address <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE - 4) ||
					(traceInstructionBuffer[i].OperationType == LoadHalfWord && (address & 0x1) == 0 && address <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE - 2) ||
					(traceInstructionBuffer[i].OperationType == LoadByte && address <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE - 1) ||
					(traceInstructionBuffer[i].OperationType == StoreWord && (address & 0x3) == 0 && address <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE - 4) ||
					(traceInstructionBuffer[i].OperationType == StoreHalfWord && (address & 0x1) == 0 && address <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE - 2) ||
					(traceInstructionBuffer[i].OperationType == StoreByte && address <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE - 1) ||
					(traceInstructionBuffer[i].OperationType == ScratchpadRead && address < CORE_SCRATCHPAD_SIZE) ||
					(traceInstructionBuffer[i].OperationType == ScratchpadWrite && address < CORE_SCRATCHPAD_SIZE) ||
					(traceInstructionBuffer[i].OperationType == Fetch && (address & 0x3) == 0 && address <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE - 4)))
				{
					cerr << endl;
					cerr << "ERROR: Invalid memory access detected!" << endl;
					cerr << endl;
					cerr << "address=" << address << endl;
					cerr << endl;
					cerr << "ISID=" << traceInstructionBuffer[i].ISID << endl;
					cerr << "ClockCycle=" << traceInstructionBuffer[i].ClockCycle << endl;
					cerr << "InstructionAddress=" << traceInstructionBuffer[i].InstructionAddress << endl;

					switch (traceInstructionBuffer[i].OperationType) {
					case LoadWord:	cerr << "OperationType=LoadWord" << endl; break;
					case LoadHalfWord:	cerr << "OperationType=LoadHalfWord" << endl; break;
					case LoadByte:	cerr << "OperationType=LoadByte" << endl; break;
					case StoreWord:	cerr << "OperationType=StoreWord" << endl; break;
					case StoreHalfWord:	cerr << "OperationType=StoreHalfWord" << endl; break;
					case StoreByte:	cerr << "OperationType=StoreByte" << endl; break;
					case ScratchpadRead:	cerr << "OperationType=ScratchpadRead" << endl; break;
					case ScratchpadWrite:	cerr << "OperationType=ScratchpadWrite" << endl; break;
					case Fetch:	cerr << "OperationType=Fetch" << endl; break;
					default:	cerr << "OperationType=Unknown (" << traceInstructionBuffer[i].OperationType << ")" << endl; break;
					}

					cerr << "Executed=" << traceInstructionBuffer[i].Executed << endl;
					cerr << "EndOfPacket=" << traceInstructionBuffer[i].EndOfPacket << endl;
					cerr << "MemoryAddress=" << traceInstructionBuffer[i].MemoryAddress << endl;
					cerr << "MemoryAddressSet=" << traceInstructionBuffer[i].MemoryAddressSet << endl;
					cerr << "MemoryData=" << traceInstructionBuffer[i].MemoryData << endl;
					cerr << "MemoryDataSet=" << traceInstructionBuffer[i].MemoryDataSet << endl;
					cerr << "InputChannel1=" << traceInstructionBuffer[i].InputChannel1 << endl;
					cerr << "InputChannel2=" << traceInstructionBuffer[i].InputChannel2 << endl;
					cerr << "UsesInputChannel1=" << traceInstructionBuffer[i].UsesInputChannel1 << endl;
					cerr << "UsesInputChannel2=" << traceInstructionBuffer[i].UsesInputChannel2 << endl;

					exit(1);
				}

				traceInstructionBuffer[i].MemoryAddress = address;
				traceInstructionBuffer[i].MemoryAddressSet = true;
				return;
			}
		}

		cerr << "Error: LBT trace ISID not found" << endl;
		exit(1);
	}
}

void setInstructionMemoryData(unsigned long long isid, unsigned long data) {
	if (traceWriter != NULL) {
		for (unsigned long i = 0; i < kTraceInstructionBufferSize; i++) {
			if (traceInstructionBuffer[i].ISID == isid) {
				assert(traceInstructionBuffer[i].OperationType == StoreWord ||
					traceInstructionBuffer[i].OperationType == StoreHalfWord ||
					traceInstructionBuffer[i].OperationType == StoreByte);

				traceInstructionBuffer[i].MemoryData = data;
				traceInstructionBuffer[i].MemoryDataSet = true;
				return;
			}
		}

		cerr << "Error: LBT trace ISID not found" << endl;
		exit(1);
	}
}

void setInstructionExecuteFlag(unsigned long long isid, bool execute) {
	if (traceWriter != NULL) {
		for (unsigned long i = 0; i < kTraceInstructionBufferSize; i++) {
			if (traceInstructionBuffer[i].ISID == isid) {
				traceInstructionBuffer[i].Executed = execute;
				return;
			}
		}

		cerr << "Error: LBT trace ISID not found" << endl;
		exit(1);
	}
}

void setInstructionSystemCallInfo(unsigned long long isid, unsigned long systemCallNumber, const unsigned long *registerValues, unsigned long registerCount) {
	if (traceWriter != NULL) {
		for (unsigned long i = 0; i < kTraceInstructionBufferSize; i++) {
			if (traceInstructionBuffer[i].ISID == isid) {
				traceInstructionBuffer[i].SystemCallNumber = systemCallNumber;

				if (registerCount > 0) {
					traceInstructionBuffer[i].RegisterValues = new ulong[registerCount];
					memcpy(traceInstructionBuffer[i].RegisterValues, registerValues, sizeof(ulong) * registerCount);
				}

				traceInstructionBuffer[i].RegisterCount = registerCount;

				traceInstructionBuffer[i].ExtraData = NULL;
				traceInstructionBuffer[i].ExtraDataLength = 0;

				traceInstructionBuffer[i].SystemCallInformationSet = true;
				return;
			}
		}

		cerr << "Error: LBT trace ISID not found" << endl;
		exit(1);
	}
}

void setInstructionSystemCallData(unsigned long long isid, const void *data, unsigned long length, bool updateMemory, unsigned long memoryAddress) {
	if (traceWriter != NULL) {
		for (unsigned long i = 0; i < kTraceInstructionBufferSize; i++) {
			if (traceInstructionBuffer[i].ISID == isid) {
				if (length > 0) {
					traceInstructionBuffer[i].ExtraData = new unsigned char[length];
					memcpy(traceInstructionBuffer[i].ExtraData, data, length);
				}

				traceInstructionBuffer[i].ExtraDataLength = length;

				if (updateMemory) {
					assert(memoryAddress + length <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
					memcpy(((uint8*)traceMemoryImage) + memoryAddress, data, length);
				}

				return;
			}
		}

		cerr << "Error: LBT trace ISID not found" << endl;
		exit(1);
	}
}

void setInstructionInputChannels(unsigned long long isid, unsigned long channel1, bool useChannel1, unsigned long channel2, bool useChannel2) {
	if (traceWriter != NULL) {
		for (unsigned long i = 0; i < kTraceInstructionBufferSize; i++) {
			if (traceInstructionBuffer[i].ISID == isid) {
				traceInstructionBuffer[i].InputChannel1 = useChannel1 ? channel1 : channel2;
				traceInstructionBuffer[i].UsesInputChannel1 = useChannel1 || useChannel2;
				traceInstructionBuffer[i].InputChannel2 = channel2;
				traceInstructionBuffer[i].UsesInputChannel2 = useChannel1 && useChannel2;
				return;
			}
		}

		cerr << "Error: LBT trace ISID not found" << endl;
		exit(1);
	}
}

// Store memory image

void storeInitialMemoryImage(const void *image) {
	if (traceWriter != NULL) {
		traceWriter->SetMemorySize(MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
		traceWriter->StoreMemoryImage(image, true);

		traceMemoryImage = new uint32[MEMORY_ON_CHIP_SCRATCHPAD_SIZE / 4];
		memcpy(traceMemoryImage, image, MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
	}
}

void storeFinalMemoryImage(const void *image) {
	if (traceWriter != NULL) {
		traceWriter->StoreMemoryImage(image, false);

		if (!traceInstructionBufferFlushed) {
			for (unsigned long i = 0; i < kTraceInstructionBufferSize; i++) {
				flushBufferedInstruction();

				traceInstructionBuffer[traceInstructionCursor].ISID = 0;

				traceInstructionCursor++;
				if (traceInstructionCursor == kTraceInstructionBufferSize)
					traceInstructionCursor = 0;
			}

			traceInstructionBufferFlushed = true;
		}

		if (memcmp(traceMemoryImage, image, MEMORY_ON_CHIP_SCRATCHPAD_SIZE) != 0) {
			cerr << "Error: Trace memory image does not match final background memory image" << endl;
			exit(1);
		}
	}
}

// Finalize trace

void stop() {
	if (traceWriter != NULL) {
		if (!traceInstructionBufferFlushed) {
			for (unsigned long i = 0; i < kTraceInstructionBufferSize; i++) {
				flushBufferedInstruction();

				traceInstructionBuffer[traceInstructionCursor].ISID = 0;

				traceInstructionCursor++;
				if (traceInstructionCursor == kTraceInstructionBufferSize)
					traceInstructionCursor = 0;
			}

			traceInstructionBufferFlushed = true;
		}

		traceWriter->Flush();
		delete traceWriter;
		delete traceFile;

		traceFile = NULL;
		traceWriter = NULL;

		delete[] traceMemoryImage;
		traceMemoryImage = NULL;
	}
}

}