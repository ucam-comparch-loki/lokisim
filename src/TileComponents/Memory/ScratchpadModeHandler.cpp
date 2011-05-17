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

#include "../../Typedefs.h"
#include "../../Utility/Instrumentation.h"
#include "ScratchpadModeHandler.h"

#include <cassert>

uint ScratchpadModeHandler::log2Exact(uint value) {
	assert(value > 1);

	uint result = 0;
	uint temp = value >> 1;

	while (temp != 0) {
		result++;
		temp >>= 1;
	}

	assert(1UL << result == value);

	return result;
}

ScratchpadModeHandler::ScratchpadModeHandler(uint bankNumber) {
	//-- Configuration parameters -----------------------------------------------------------------

	cSetCount = MEMORY_CACHE_SET_COUNT;
	cWayCount = MEMORY_CACHE_WAY_COUNT;
	cLineSize = MEMORY_CACHE_LINE_SIZE;

	//-- State ------------------------------------------------------------------------------------

	mData = new uint32_t[cSetCount * cWayCount * cLineSize / 4];

	mLineBits = log2Exact(cLineSize);
	mLineMask = (1UL << mLineBits) - 1UL;

	mGroupIndex = 0;
	mGroupBits = 0;
	mGroupMask = 0;

	mBankNumber = bankNumber;
}

ScratchpadModeHandler::~ScratchpadModeHandler() {
	delete[] mData;
}

void ScratchpadModeHandler::activate(uint groupIndex, uint groupSize) {
	memset(mData, 0x00, cSetCount * cWayCount * cLineSize / 4);

	mGroupIndex = groupIndex;
	mGroupBits = (groupSize == 1) ? 0 : log2Exact(groupSize);
	mGroupMask = (groupSize == 1) ? 0 : (((1UL << mGroupBits) - 1UL) << mLineBits);
}

bool ScratchpadModeHandler::containsAddress(uint32_t address) {
	assert(address < cSetCount * cWayCount * cLineSize * (1UL << mGroupBits));
	return (address & mGroupMask) == (mGroupIndex << mLineBits);
}

uint32_t ScratchpadModeHandler::readWord(uint32_t address) {
	assert(address < cSetCount * cWayCount * cLineSize * (1UL << mGroupBits));
	assert((address & 0x3) == 0);
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	Instrumentation::memoryReadWord(mBankNumber, address, false);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= cSetCount * cWayCount * cLineSize);
	return mData[slot / 4];
}

uint32_t ScratchpadModeHandler::readHalfWord(uint32_t address) {
	assert(address < cSetCount * cWayCount * cLineSize * (1UL << mGroupBits));
	assert((address & 0x1) == 0);
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	Instrumentation::memoryReadHalfWord(mBankNumber, address, false);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= cSetCount * cWayCount * cLineSize);
	uint32_t data = mData[slot / 4];
	return ((address & 0x2) == 0) ? (data & 0xFFFFUL) : (data >> 16);	// Little endian
}

uint32_t ScratchpadModeHandler::readByte(uint32_t address) {
	assert(address < cSetCount * cWayCount * cLineSize * (1UL << mGroupBits));
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	Instrumentation::memoryReadByte(mBankNumber, address, false);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= cSetCount * cWayCount * cLineSize);
	uint32_t data = mData[slot / 4];

	switch (address & 0x3) {	// Little endian
	case 0:	return data & 0xFFUL;
	case 1:	return (data >> 8) & 0xFFUL;
	case 2:	return (data >> 16) & 0xFFUL;
	case 3:	return (data >> 24) & 0xFFUL;
	}

	return 0;  // Silence compiler warning
}

void ScratchpadModeHandler::writeWord(uint32_t address, uint32_t data) {
	assert(address < cSetCount * cWayCount * cLineSize * (1UL << mGroupBits));
	assert((address & 0x3) == 0);
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	Instrumentation::memoryWriteWord(mBankNumber, address, false);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= cSetCount * cWayCount * cLineSize);
	mData[slot / 4] = data;
}

void ScratchpadModeHandler::writeHalfWord(uint32_t address, uint32_t data) {
	assert(address < cSetCount * cWayCount * cLineSize * (1UL << mGroupBits));
	assert((address & 0x1) == 0);
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	Instrumentation::memoryWriteHalfWord(mBankNumber, address, false);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= cSetCount * cWayCount * cLineSize);
	uint32_t oldData = mData[slot / 4];

	if ((address & 0x2) == 0)	// Little endian
		mData[slot / 4] = (oldData & 0xFFFF0000UL) | (data & 0x0000FFFFUL);
	else
		mData[slot / 4] = (oldData & 0x0000FFFFUL) | (data << 16);
}

void ScratchpadModeHandler::writeByte(uint32_t address, uint32_t data) {
	assert(address < cSetCount * cWayCount * cLineSize * (1UL << mGroupBits));
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	Instrumentation::memoryWriteByte(mBankNumber, address, false);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= cSetCount * cWayCount * cLineSize);
	uint32_t oldData = mData[slot / 4];

	// Are breaks needed here?
	switch (address & 0x3) {	// Little endian
	case 0:	mData[slot / 4] = (oldData & 0xFFFFFF00UL) | (data & 0x000000FFUL);
	case 1:	mData[slot / 4] = (oldData & 0xFFFF00FFUL) | ((data & 0x000000FFUL) << 8);
	case 2:	mData[slot / 4] = (oldData & 0xFF00FFFFUL) | ((data & 0x000000FFUL) << 16);
	case 3:	mData[slot / 4] = (oldData & 0x00FFFFFFUL) | ((data & 0x000000FFUL) << 24);
	}
}
