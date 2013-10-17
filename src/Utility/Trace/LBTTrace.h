/*
 * LBTTrace.h
 *
 *  Created on: 9 Sep 2013
 *      Author: afjk2
 */

#ifndef LBTTRACE_H_
#define LBTTRACE_H_

#include <string>

#include "../Parameters.h"

namespace LBTTrace {

	// Initialize trace writer

	void start(const std::string& filename);

	// Update clock cycle

	void setClockCycle(unsigned long long cycleNumber);

	// Log activity

	enum LBTOperationType {
		NOP,
		ALU1,
		ALU2,
		Fetch,
		ScratchpadRead,
		ScratchpadWrite,
		LoadImmediate,
		SystemCall,
		Control,
		LoadWord,
		LoadHalfWord,
		LoadByte,
		StoreWord,
		StoreHalfWord,
		StoreByte
	};

	unsigned long long logDecodedInstruction(unsigned long instructionAddress, LBTOperationType operationType, bool endOfPacket);

	void setInstructionMemoryAddress(unsigned long long isid, unsigned long address);
	void setInstructionMemoryData(unsigned long long isid, unsigned long data);
	void setInstructionExecuteFlag(unsigned long long isid, bool execute);
	void setInstructionSystemCallInfo(unsigned long long isid, unsigned long systemCallNumber, const unsigned long *registerValues, unsigned long registerCount);
	void setInstructionSystemCallData(unsigned long long isid, const void *data, unsigned long length, bool updateMemory, unsigned long memoryAddress);
	void setInstructionInputChannels(unsigned long long isid, unsigned long channel1, bool useChannel1, unsigned long channel2, bool useChannel2);

	// Store memory image

	void storeInitialMemoryImage(const void *image);
	void storeFinalMemoryImage(const void *image);

	// Finalize trace

	void stop();

}

#endif /* LBTTRACE_H_ */
