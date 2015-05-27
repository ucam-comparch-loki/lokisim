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

	virtual bool containsAddress(MemoryAddr address);
	virtual bool sameLine(MemoryAddr address1, MemoryAddr address2);

  virtual bool readWord(MemoryAddr address, uint32_t &data, bool instruction, bool resume, bool debug, int core, int retCh);
  virtual bool readHalfWord(MemoryAddr address, uint32_t &data, bool resume, bool debug, int core, int retCh);
  virtual bool readByte(MemoryAddr address, uint32_t &data, bool resume, bool debug, int core, int retCh);

  virtual bool writeWord(MemoryAddr address, uint32_t data, bool resume, bool debug, int core, int retCh);
  virtual bool writeHalfWord(MemoryAddr address, uint32_t data, bool resume, bool debug, int core, int retCh);
  virtual bool writeByte(MemoryAddr address, uint32_t data, bool resume, bool debug, int core, int retCh);
};

#endif /* SCRATCHPADMODEHANDLER_H_ */
