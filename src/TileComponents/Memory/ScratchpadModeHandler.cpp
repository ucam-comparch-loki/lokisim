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

ScratchpadModeHandler::ScratchpadModeHandler(uint bankNumber) :
    AbstractMemoryHandler(bankNumber) {
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

bool ScratchpadModeHandler::readWord(MemoryAddr address, uint32_t &data, bool instruction, bool resume, bool debug, int core, int retCh) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));
	if ((address & 0x3) != 0)
	  throw UnalignedAccessException(address, 4);

	if (!debug && ENERGY_TRACE) {
	  if (instruction)
      Instrumentation::MemoryBank::readIPKWord(mBankNumber, address, false, core, retCh);
    else
      Instrumentation::MemoryBank::readWord(mBankNumber, address, false, core, retCh);
	}

	if (!debug && Arguments::memoryTrace()) {
		if (instruction)
			MemoryTrace::readIPKWord(mBankNumber, address);
		else
			MemoryTrace::readWord(mBankNumber, address);
	}

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= mSetCount * mWayCount * mLineSize);
	data = mData[slot / 4];

	return true;
}

bool ScratchpadModeHandler::readHalfWord(MemoryAddr address, uint32_t &data, bool resume, bool debug, int core, int retCh) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));
  if ((address & 0x1) != 0)
    throw UnalignedAccessException(address, 2);

	if (!debug && ENERGY_TRACE)
	  Instrumentation::MemoryBank::readHalfWord(mBankNumber, address, false, core, retCh);

	if (!debug && Arguments::memoryTrace())
		MemoryTrace::readHalfWord(mBankNumber, address);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= mSetCount * mWayCount * mLineSize);
	uint32_t dataWord = mData[slot / 4];
	data = ((address & 0x3) == 0) ? (dataWord & 0xFFFFUL) : (dataWord >> 16);	// Little endian

	return true;
}

bool ScratchpadModeHandler::readByte(MemoryAddr address, uint32_t &data, bool resume, bool debug, int core, int retCh) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));

	if (!debug && ENERGY_TRACE)
	  Instrumentation::MemoryBank::readByte(mBankNumber, address, false, core, retCh);

	if (!debug && Arguments::memoryTrace())
		MemoryTrace::readByte(mBankNumber, address);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= mSetCount * mWayCount * mLineSize);
	uint32_t dataWord = mData[slot / 4];
	uint32_t selector = address & 0x3UL;

	switch (selector) {	// Little endian
	case 0:	data = dataWord & 0xFFUL;			break;
	case 1:	data = (dataWord >> 8) & 0xFFUL;	break;
	case 2:	data = (dataWord >> 16) & 0xFFUL;	break;
	case 3:	data = (dataWord >> 24) & 0xFFUL;	break;
	}

	return true;
}

bool ScratchpadModeHandler::writeWord(MemoryAddr address, uint32_t data, bool resume, bool debug, int core, int retCh) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));
  if ((address & 0x3) != 0)
    throw UnalignedAccessException(address, 4);

	if (!debug && ENERGY_TRACE)
	  Instrumentation::MemoryBank::writeWord(mBankNumber, address, false, core, retCh);

	if (!debug && Arguments::memoryTrace())
		MemoryTrace::writeWord(mBankNumber, address);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= mSetCount * mWayCount * mLineSize);
	mData[slot / 4] = data;

	return true;
}

bool ScratchpadModeHandler::writeHalfWord(MemoryAddr address, uint32_t data, bool resume, bool debug, int core, int retCh) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));
  if ((address & 0x1) != 0)
    throw UnalignedAccessException(address, 2);

	if (!debug && ENERGY_TRACE)
	  Instrumentation::MemoryBank::writeHalfWord(mBankNumber, address, false, core, retCh);

	if (!debug && Arguments::memoryTrace())
		MemoryTrace::writeHalfWord(mBankNumber, address);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= mSetCount * mWayCount * mLineSize);
	uint32_t oldData = mData[slot / 4];

	if ((address & 0x3) == 0)	// Little endian
		mData[slot / 4] = (oldData & 0xFFFF0000UL) | (data & 0x0000FFFFUL);
	else
		mData[slot / 4] = (oldData & 0x0000FFFFUL) | (data << 16);

	return true;
}

bool ScratchpadModeHandler::writeByte(MemoryAddr address, uint32_t data, bool resume, bool debug, int core, int retCh) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));

	if (!debug && ENERGY_TRACE)
	  Instrumentation::MemoryBank::writeByte(mBankNumber, address, false, core, retCh);

	if (!debug && Arguments::memoryTrace())
		MemoryTrace::writeByte(mBankNumber, address);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= mSetCount * mWayCount * mLineSize);
	uint32_t oldData = mData[slot / 4];
	uint32_t selector = address & 0x3UL;

	// Are breaks needed here?
	switch (selector) {	// Little endian
	case 0:	mData[slot / 4] = (oldData & 0xFFFFFF00UL) | (data & 0x000000FFUL);				break;
	case 1:	mData[slot / 4] = (oldData & 0xFFFF00FFUL) | ((data & 0x000000FFUL) << 8);		break;
	case 2:	mData[slot / 4] = (oldData & 0xFF00FFFFUL) | ((data & 0x000000FFUL) << 16);		break;
	case 3:	mData[slot / 4] = (oldData & 0x00FFFFFFUL) | ((data & 0x000000FFUL) << 24);		break;
	}

	return true;
}
