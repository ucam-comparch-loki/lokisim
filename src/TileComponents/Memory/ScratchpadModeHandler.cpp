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

	mSetCount = MEMORY_CACHE_SET_COUNT;
	mWayCount = MEMORY_CACHE_WAY_COUNT;
	mLineSize = MEMORY_CACHE_LINE_SIZE;

	//-- State ------------------------------------------------------------------------------------

	mData = new uint32_t[mSetCount * mWayCount * mLineSize / 4];

	mLineBits = log2Exact(mLineSize);
	mLineMask = (1UL << mLineBits) - 1UL;

	mGroupIndex = 0;
	mGroupBits = 0;
	mGroupMask = 0;

	mBankNumber = bankNumber;
}

ScratchpadModeHandler::~ScratchpadModeHandler() {
	delete[] mData;
}

void ScratchpadModeHandler::activate(uint groupIndex, uint groupSize, uint wayCount, uint lineSize) {
	mSetCount = MEMORY_CACHE_SET_COUNT * MEMORY_CACHE_WAY_COUNT * MEMORY_CACHE_LINE_SIZE / (wayCount * lineSize);
	mWayCount = wayCount;
	mLineSize = lineSize;

	assert(log2Exact(mSetCount) >= 2);
	assert(mWayCount == 1 || log2Exact(mWayCount) >= 1);
	assert(log2Exact(mLineSize) >= 2);

	memset(mData, 0x00, mSetCount * mWayCount * mLineSize / 4);

	mLineBits = log2Exact(mLineSize);
	mLineMask = (1UL << mLineBits) - 1UL;

	mGroupIndex = groupIndex;
	mGroupBits = (groupSize == 1) ? 0 : log2Exact(groupSize);
	mGroupMask = (groupSize == 1) ? 0 : (((1UL << mGroupBits) - 1UL) << mLineBits);

	Instrumentation::memorySetMode(mBankNumber, false, mSetCount, mWayCount, mLineSize);
}

bool ScratchpadModeHandler::containsAddress(uint32_t address) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));
	return (address & mGroupMask) == (mGroupIndex << mLineBits);
}

uint32_t ScratchpadModeHandler::readWord(uint32_t address, bool instruction) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));
	assert((address & 0x3) == 0);
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	if (instruction)
		Instrumentation::memoryReadIPKWord(mBankNumber, address, false);
	else
		Instrumentation::memoryReadWord(mBankNumber, address, false);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= mSetCount * mWayCount * mLineSize);
	return mData[slot / 4];
}

uint32_t ScratchpadModeHandler::readHalfWord(uint32_t address) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));
	assert((address & 0x1) == 0);
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	Instrumentation::memoryReadHalfWord(mBankNumber, address, false);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= mSetCount * mWayCount * mLineSize);
	uint32_t data = mData[slot / 4];
	return ((address & 0x2) == 0) ? (data & 0xFFFFUL) : (data >> 16);	// Little endian
}

uint32_t ScratchpadModeHandler::readByte(uint32_t address) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	Instrumentation::memoryReadByte(mBankNumber, address, false);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= mSetCount * mWayCount * mLineSize);
	uint32_t data = mData[slot / 4];
	uint32_t selector = address & 0x3UL;

	switch (selector) {	// Little endian
	case 0:	return data & 0xFFUL;			break;
	case 1:	return (data >> 8) & 0xFFUL;	break;
	case 2:	return (data >> 16) & 0xFFUL;	break;
	case 3:	return (data >> 24) & 0xFFUL;	break;
	}

	return 0;  // Silence compiler warning
}

void ScratchpadModeHandler::writeWord(uint32_t address, uint32_t data) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));
	assert((address & 0x3) == 0);
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	Instrumentation::memoryWriteWord(mBankNumber, address, false);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= mSetCount * mWayCount * mLineSize);
	mData[slot / 4] = data;
}

void ScratchpadModeHandler::writeHalfWord(uint32_t address, uint32_t data) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));
	assert((address & 0x1) == 0);
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	Instrumentation::memoryWriteHalfWord(mBankNumber, address, false);

	uint32_t slot = (address & mLineMask) | ((address >> mGroupBits) & ~mLineMask);
	assert(slot <= mSetCount * mWayCount * mLineSize);
	uint32_t oldData = mData[slot / 4];

	if ((address & 0x2) == 0)	// Little endian
		mData[slot / 4] = (oldData & 0xFFFF0000UL) | (data & 0x0000FFFFUL);
	else
		mData[slot / 4] = (oldData & 0x0000FFFFUL) | (data << 16);
}

void ScratchpadModeHandler::writeByte(uint32_t address, uint32_t data) {
	assert(address < mSetCount * mWayCount * mLineSize * (1UL << mGroupBits));
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	Instrumentation::memoryWriteByte(mBankNumber, address, false);

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
}
