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

// Internal helper routine

void flushBufferedInstruction() {
	if (traceInstructionBuffer[traceInstructionCursor].ISID != 0 && !traceExitCallReached) {
		// Write pending instruction

		switch (traceInstructionBuffer[traceInstructionCursor].OperationType) {
		case NOP:
			traceWriter->AddBasicOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkRecord::kOperationTypeNOP,
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
				SLBTChunkRecord::kOperationTypeALU1,
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
				SLBTChunkRecord::kOperationTypeALU2,
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
				SLBTChunkRecord::kOperationTypeFetch,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);
			break;
		case ScratchpadRead:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkRecord::kOperationTypeScratchpadRead,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);
			break;
		case ScratchpadWrite:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkRecord::kOperationTypeScratchpadWrite,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);
			break;
		case LoadImmediate:
			traceWriter->AddBasicOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkRecord::kOperationTypeLoadImmediate,
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
				SLBTChunkRecord::kOperationTypeControl,
				traceInstructionBuffer[traceInstructionCursor].InputChannel1,
				traceInstructionBuffer[traceInstructionCursor].UsesInputChannel1,
				traceInstructionBuffer[traceInstructionCursor].InputChannel2,
				traceInstructionBuffer[traceInstructionCursor].UsesInputChannel2,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);
			break;
		case LoadWord:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkRecord::kOperationTypeLoadWord,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);
			break;
		case LoadHalfWord:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkRecord::kOperationTypeLoadHalfWord,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);
			break;
		case LoadByte:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkRecord::kOperationTypeLoadByte,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);
			break;
		case StoreWord:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkRecord::kOperationTypeStoreWord,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);
			break;
		case StoreHalfWord:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkRecord::kOperationTypeStoreHalfWord,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
				traceInstructionBuffer[traceInstructionCursor].Executed,
				traceInstructionBuffer[traceInstructionCursor].EndOfPacket);
			break;
		case StoreByte:
			assert(traceInstructionBuffer[traceInstructionCursor].MemoryAddressSet);
			traceWriter->AddMemoryOperation(
				traceInstructionBuffer[traceInstructionCursor].ClockCycle,
				traceInstructionBuffer[traceInstructionCursor].InstructionAddress,
				SLBTChunkRecord::kOperationTypeStoreByte,
				traceInstructionBuffer[traceInstructionCursor].MemoryAddress,
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
}

// Update clock cycle

void setClockCycle(unsigned long long cycleNumber) {
	traceClockCycle = cycleNumber;
}

// Log activity

unsigned long long logDecodedInstruction(unsigned long instructionAddress, LBTOperationType operationType, bool endOfPacket) {
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
				traceInstructionBuffer[i].MemoryAddress = address;
				traceInstructionBuffer[i].MemoryAddressSet = true;
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

void setInstructionSystemCallInfo(unsigned long long isid, unsigned long systemCallNumber, const unsigned long *registerValues, unsigned long registerCount, const void *extraData, unsigned long extraDataLength) {
	if (traceWriter != NULL) {
		for (unsigned long i = 0; i < kTraceInstructionBufferSize; i++) {
			if (traceInstructionBuffer[i].ISID == isid) {
				traceInstructionBuffer[i].SystemCallNumber = systemCallNumber;

				if (registerCount > 0) {
					traceInstructionBuffer[i].RegisterValues = new ulong[registerCount];
					memcpy(traceInstructionBuffer[i].RegisterValues, registerValues, sizeof(ulong) * registerCount);
				}

				traceInstructionBuffer[i].RegisterCount = registerCount;

				if (extraDataLength > 0) {
					traceInstructionBuffer[i].ExtraData = new unsigned char[extraDataLength];
					memcpy(traceInstructionBuffer[i].ExtraData, extraData, extraDataLength);
				}

				traceInstructionBuffer[i].ExtraDataLength = extraDataLength;

				traceInstructionBuffer[i].SystemCallInformationSet = true;
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

// Finalize trace

void stop() {
	if (traceWriter != NULL) {
		for (unsigned long i = 0; i < kTraceInstructionBufferSize; i++) {
			flushBufferedInstruction();

			traceInstructionBuffer[traceInstructionCursor].ISID = 0;

			traceInstructionCursor++;
			if (traceInstructionCursor == kTraceInstructionBufferSize)
				traceInstructionCursor = 0;
		}

		traceWriter->Flush();
		delete traceWriter;
		delete traceFile;

		traceFile = NULL;
		traceWriter = NULL;
	}
}

}
