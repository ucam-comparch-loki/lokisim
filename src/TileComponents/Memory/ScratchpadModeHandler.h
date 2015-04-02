//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Scratchpad Mode Handler Module Definition
//-------------------------------------------------------------------------------------------------
// Models the logic and memory active in the scratchpad mode of a memory bank.
//
// The model is used internally by the memory bank model and is not a SystemC
// module by itself.
//-------------------------------------------------------------------------------------------------
// File:       ScratchpadMode.h
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 11/04/2011
//-------------------------------------------------------------------------------------------------

#ifndef SCRATCHPADMODEHANDLER_H_
#define SCRATCHPADMODEHANDLER_H_

#include "../../Typedefs.h"

class ScratchpadModeHandler {
private:
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

	uint mSetCount;							// Number of sets in general purpose cache mode
	uint mWayCount;							// Number of ways in general purpose cache mode
	uint mLineSize;							// Size of lines (for cache management and data interleaving)

	//---------------------------------------------------------------------------------------------
	// State
	//---------------------------------------------------------------------------------------------

	uint32_t *mData;						// Data words stored in the scratchpad

	uint mLineBits;							// Number of line bits - log2(line size)
	uint32_t mLineMask;						// Bit mask used to extract line bits

	uint mGroupIndex;						// Relative index of bank within group (off by one)
	uint mGroupBits;						// Number of group bits - log2(group size)
	uint32_t mGroupMask;					// Bit mask used to extract group bits

	uint mBankNumber;						// Bank number for statistics use

	//---------------------------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------------------------

	uint log2Exact(uint value);

public:
	ScratchpadModeHandler(uint bankNumber);
	~ScratchpadModeHandler();

	void activate(uint groupIndex, uint groupSize, uint wayCount, uint lineSize);

	bool containsAddress(uint32_t address);
	bool sameLine(uint32_t address1, uint32_t address2);

	uint32_t readWord(uint32_t address, bool instruction, int core, int retCh);
	uint32_t readHalfWord(uint32_t address, int core, int retCh);
	uint32_t readByte(uint32_t address, int core, int retCh);

	void writeWord(uint32_t address, uint32_t data, int core, int retCh);
	void writeHalfWord(uint32_t address, uint32_t data, int core, int retCh);
	void writeByte(uint32_t address, uint32_t data, int core, int retCh);
};

#endif /* SCRATCHPADMODEHANDLER_H_ */
