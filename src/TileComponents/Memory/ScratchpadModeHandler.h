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

#include <vector>
#include "AbstractMemoryHandler.h"

using std::vector;

class ScratchpadModeHandler : public AbstractMemoryHandler {

	//---------------------------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------------------------

public:
	ScratchpadModeHandler(uint bankNumber, vector<uint32_t>& data);
	~ScratchpadModeHandler();

	virtual void activate(const MemoryConfig& config);
};

#endif /* SCRATCHPADMODEHANDLER_H_ */
