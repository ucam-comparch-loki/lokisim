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

#include "AbstractMemoryHandler.h"

class ScratchpadModeHandler : public AbstractMemoryHandler {

	//---------------------------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------------------------

public:
	ScratchpadModeHandler(uint bankNumber);
	~ScratchpadModeHandler();

	virtual void activate(const MemoryConfig& config);

	virtual bool containsAddress(uint32_t address);
	virtual bool sameLine(uint32_t address1, uint32_t address2);

	uint32_t readWord(uint32_t address, bool instruction);
	uint32_t readHalfWord(uint32_t address);
	uint32_t readByte(uint32_t address);

	void writeWord(uint32_t address, uint32_t data);
	void writeHalfWord(uint32_t address, uint32_t data);
	void writeByte(uint32_t address, uint32_t data);
};

#endif /* SCRATCHPADMODEHANDLER_H_ */
