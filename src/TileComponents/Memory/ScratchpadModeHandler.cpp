//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Scratchpad Mode Handler Module Implementation
//-------------------------------------------------------------------------------------------------
// Models the logic and memory active in the scratchpad mode of a memory bank.
//
// The model is used internally by the memory bank model and is not a SystemC
// module by itself.
//-------------------------------------------------------------------------------------------------
// File:       ScratchpadMode.cpp
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 11/04/2011
//-------------------------------------------------------------------------------------------------

#include <cassert>
#include "ScratchpadModeHandler.h"
#include "../../Exceptions/UnalignedAccessException.h"
#include "../../Utility/Instrumentation/MemoryBank.h"
#include "../../Utility/Trace/MemoryTrace.h"

ScratchpadModeHandler::ScratchpadModeHandler(uint bankNumber, vector<uint32_t>& data) :
    AbstractMemoryHandler(bankNumber, data) {
  // Do nothing
}

ScratchpadModeHandler::~ScratchpadModeHandler() {
  // Do nothing
}

void ScratchpadModeHandler::activate(const MemoryConfig& config) {
	mSetCount = MEMORY_BANK_SIZE / (config.WayCount * config.LineSize);
	mWayCount = config.WayCount;
	mLineSize = config.LineSize;

	assert(log2Exact(mSetCount) >= 2);
	assert(mWayCount == 1 || log2Exact(mWayCount) >= 1);
	assert(log2Exact(mLineSize) >= 2);

	memset(mData, 0x00, mSetCount * mWayCount * mLineSize / 4);

	mLineBits = log2Exact(mLineSize);
	mLineMask = (1UL << mLineBits) - 1UL;

	mGroupBits = (config.GroupSize == 1) ? 0 : log2Exact(config.GroupSize);
	mGroupMask = (config.GroupSize == 1) ? 0 : (((1UL << mGroupBits) - 1UL) << mLineBits);

	if (ENERGY_TRACE)
	  Instrumentation::MemoryBank::setMode(mBankNumber, false, mSetCount, mWayCount, mLineSize);
}
