//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// General Purpose Cache Mode Handler Module Implementation
//-------------------------------------------------------------------------------------------------
// Models the logic and memory active in the general purpose cache mode of a
// memory bank.
//
// The model is used internally by the memory bank model and is not a SystemC
// module by itself.
//-------------------------------------------------------------------------------------------------
// File:       GeneralPurposeCacheHandler.cpp
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 11/04/2011
//-------------------------------------------------------------------------------------------------

#include "../../Typedefs.h"
#include "../../Utility/Instrumentation/MemoryBank.h"
#include "../../Utility/Trace/MemoryTrace.h"
#include "SimplifiedOnChipScratchpad.h"
#include "GeneralPurposeCacheHandler.h"

#include <cassert>
#include "../../Exceptions/ReadOnlyException.h"

uint GeneralPurposeCacheHandler::log2Exact(uint value) {
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

bool GeneralPurposeCacheHandler::lookupCacheLine(uint32_t address, uint &slot, bool resume, bool read, bool instruction) {
	assert((address & mGroupMask) == (mGroupIndex << mLineBits));

	uint32_t lineAddress = address & ~mLineMask;
	uint setIndex = (address & mSetMask) >> mSetShift;
	uint startSlot = setIndex * mWayCount;
	bool hit = false;

	for (uint i = 0; i < mWayCount; i++) {
		if (mLineValid[startSlot + i] && mAddresses[startSlot + i] == lineAddress) {
			slot = startSlot + i;

			hit = true;
			break;
		}
	}

	if (Arguments::csimTrace() && !resume)
    printf("MEM%d 0x%08x: %s %s %s\n", this->mBankNumber,
                                       address,
                                       instruction ? "instruction" : "data",
                                       read ? "read" : "write",
                                       hit ? "hit" : "miss");

	return hit;
}

void GeneralPurposeCacheHandler::promoteCacheLine(uint slot) {
	if (!cRandomReplacement) {
		// Physically swap data in the model to simplify ideal LRU implementation

		assert(mLineSize <= 256);
		uint setIndex = slot / mWayCount;
		uint slotLimit = setIndex * mWayCount;

		while (slot > slotLimit) {
			uint32_t tempData[64];
			memcpy(tempData, &mData[slot * mLineSize / 4], mLineSize);
			memcpy(&mData[slot * mLineSize / 4], &mData[(slot - 1) * mLineSize / 4], mLineSize);
			memcpy(&mData[(slot - 1) * mLineSize / 4], tempData, mLineSize);

			uint32_t tempAddress = mAddresses[slot];
			mAddresses[slot] = mAddresses[slot - 1];
			mAddresses[slot - 1] = tempAddress;

			bool tempValid = mLineValid[slot];
			assert(tempValid);
			mLineValid[slot] = mLineValid[slot - 1];
			mLineValid[slot - 1] = tempValid;

			bool tempDirty = mLineDirty[slot];
			mLineDirty[slot] = mLineDirty[slot - 1];
			mLineDirty[slot - 1] = tempDirty;

			slot--;
		}
	}
}

GeneralPurposeCacheHandler::GeneralPurposeCacheHandler(uint bankNumber) {
	//-- Configuration parameters -----------------------------------------------------------------

	mSetCount = mWayCount = mLineSize = 0;
	cRandomReplacement = MEMORY_CACHE_RANDOM_REPLACEMENT != 0;

	//-- State ------------------------------------------------------------------------------------

	mData = new uint32_t[MEMORY_BANK_SIZE / 4];
	mAddresses = new uint32_t[1];
	mLineValid = new bool[1];
	mLineDirty = new bool[1];

	mLFSRState = 0xFFFFU;

	mVictimSlot = mSetBits = mSetMask = mSetShift = 0;
	mLineBits = mLineMask = mGroupIndex = mGroupBits = mGroupMask = 0;

	mBankNumber = bankNumber;
	mBackgroundMemory = NULL;
}

GeneralPurposeCacheHandler::~GeneralPurposeCacheHandler() {
	delete[] mData;
	delete[] mAddresses;
	delete[] mLineValid;
	delete[] mLineDirty;
}

void GeneralPurposeCacheHandler::activate(uint groupIndex, uint groupSize, uint wayCount, uint lineSize) {
	mSetCount = MEMORY_BANK_SIZE / (wayCount * lineSize);
	mWayCount = wayCount;
	mLineSize = lineSize;

	assert(log2Exact(mSetCount) >= 2);
	assert(mWayCount == 1 || log2Exact(mWayCount) >= 1);
	assert(log2Exact(mLineSize) >= 2);

	delete[] mAddresses;
	delete[] mLineValid;
	delete[] mLineDirty;

	mAddresses = new uint32_t[mSetCount * mWayCount];
	mLineValid = new bool[mSetCount * mWayCount];
	mLineDirty = new bool[mSetCount * mWayCount];

	memset(mLineValid, 0x00, mSetCount * mWayCount * sizeof(bool));

	mVictimSlot = 0;

	mLineBits = log2Exact(mLineSize);
	mLineMask = (1UL << mLineBits) - 1UL;

	mGroupIndex = groupIndex;
	mGroupBits = (groupSize == 1) ? 0 : log2Exact(groupSize);
	mGroupMask = (groupSize == 1) ? 0 : (((1UL << mGroupBits) - 1UL) << mLineBits);

	mSetBits = log2Exact(mSetCount);
	mSetShift = mGroupBits + mLineBits;
	mSetMask = ((1UL << mSetBits) - 1UL) << mSetShift;

	if (ENERGY_TRACE)
	  Instrumentation::MemoryBank::setMode(mBankNumber, true, mSetCount, mWayCount, mLineSize);
}

bool GeneralPurposeCacheHandler::containsAddress(uint32_t address) {
	return (address & mGroupMask) == (mGroupIndex << mLineBits);
}

bool GeneralPurposeCacheHandler::sameLine(uint32_t address1, uint32_t address2) {
	assert((address1 & mGroupMask) == (mGroupIndex << mLineBits));
	assert((address2 & mGroupMask) == (mGroupIndex << mLineBits));
	return (address1 & mSetMask) == (address2 & mSetMask);
}

bool GeneralPurposeCacheHandler::readWord(uint32_t address, uint32_t &data, bool instruction, bool resume, bool debug, int core, int retCh) {
	assert((address & 0x3) == 0);

	uint slot;
	if (!lookupCacheLine(address, slot, resume, true, instruction)) {
		assert(!resume);

		if (!debug) {
			if (instruction)
				Instrumentation::MemoryBank::readIPKWord(mBankNumber, address, true, core, retCh);
			else
				Instrumentation::MemoryBank::readWord(mBankNumber, address, true, core, retCh);
		}

		if (Arguments::memoryTrace() && !debug) {
			if (instruction)
				MemoryTrace::readIPKWord(mBankNumber, address);
			else
				MemoryTrace::readWord(mBankNumber, address);
		}

		return false;
	}

	if (!resume && !debug) {
		if (instruction)
			Instrumentation::MemoryBank::readIPKWord(mBankNumber, address, false, core, retCh);
		else
			Instrumentation::MemoryBank::readWord(mBankNumber, address, false, core, retCh);
	}

	if (Arguments::memoryTrace() && !resume && !debug) {
		if (instruction)
			MemoryTrace::readIPKWord(mBankNumber, address);
		else
			MemoryTrace::readWord(mBankNumber, address);
	}

	data = mData[slot * mLineSize / 4 + (address & mLineMask) / 4];
	promoteCacheLine(slot);

	//if (mBankNumber >= 4)
	//	fprintf(stderr, "GPCH: bank %u read word %.8X from address %.8X\n", mBankNumber, data, address);

	return true;
}

bool GeneralPurposeCacheHandler::readHalfWord(uint32_t address, uint32_t &data, bool resume, bool debug, int core, int retCh) {
	assert((address & 0x1) == 0);

	uint slot;
	if (!lookupCacheLine(address, slot, resume, true, false)) {
		assert(!resume);
		if (!debug)
			Instrumentation::MemoryBank::readHalfWord(mBankNumber, address, true, core, retCh);
		if (Arguments::memoryTrace() && !debug)
			MemoryTrace::readHalfWord(mBankNumber, address);
		return false;
	}

	if (!resume && !debug)
		Instrumentation::MemoryBank::readHalfWord(mBankNumber, address, false, core, retCh);

	if (Arguments::memoryTrace() && !resume && !debug)
		MemoryTrace::readHalfWord(mBankNumber, address);

	uint32_t fullWord = mData[slot * mLineSize / 4 + (address & mLineMask) / 4];
	data = ((address & 0x3) == 0) ? (fullWord & 0xFFFFUL) : (fullWord >> 16);	// Little endian
	promoteCacheLine(slot);

	//fprintf(stderr, "GPCH: bank %u read half-word %.4X from address %.8X\n", mBankNumber, data & 0xFFFF, address);

	return true;
}

bool GeneralPurposeCacheHandler::readByte(uint32_t address, uint32_t &data, bool resume, bool debug, int core, int retCh) {
	uint slot;
	if (!lookupCacheLine(address, slot, resume, true, false)) {
		assert(!resume);
		if (!debug)
			Instrumentation::MemoryBank::readByte(mBankNumber, address, true, core, retCh);
		if (Arguments::memoryTrace() && !debug)
			MemoryTrace::readByte(mBankNumber, address);
		return false;
	}

	if (!resume && !debug)
		Instrumentation::MemoryBank::readByte(mBankNumber, address, false, core, retCh);

	if (Arguments::memoryTrace() && !resume && !debug)
		MemoryTrace::readByte(mBankNumber, address);

	uint32_t fullWord = mData[slot * mLineSize / 4 + (address & mLineMask) / 4];
	uint32_t selector = address & 0x3UL;

	switch (selector) {	// Little endian
	case 0:	data = fullWord & 0xFFUL;			break;
	case 1:	data = (fullWord >> 8) & 0xFFUL;	break;
	case 2:	data = (fullWord >> 16) & 0xFFUL;	break;
	case 3:	data = (fullWord >> 24) & 0xFFUL;	break;
	}

	promoteCacheLine(slot);

	//fprintf(stderr, "GPCH: bank %u read byte %.2X from address %.8X\n", mBankNumber, data & 0xFF, address);

	return true;
}

bool GeneralPurposeCacheHandler::writeWord(uint32_t address, uint32_t data, bool resume, bool debug, int core, int retCh) {
	assert((address & 0x3) == 0);
	if (mBackgroundMemory->readOnly(address))
	  throw ReadOnlyException(address);

	uint slot;
	if (!lookupCacheLine(address, slot, resume, false, false)) {
		assert(!resume);
		if (!debug)
			Instrumentation::MemoryBank::writeWord(mBankNumber, address, true, core, retCh);
		if (Arguments::memoryTrace() && !debug)
			MemoryTrace::writeWord(mBankNumber, address);
		return false;
	}

	if (!resume && !debug)
		Instrumentation::MemoryBank::writeWord(mBankNumber, address, false, core, retCh);

	if (Arguments::memoryTrace() && !resume && !debug)
		MemoryTrace::writeWord(mBankNumber, address);

	mData[slot * mLineSize / 4 + (address & mLineMask) / 4] = data;
	mLineDirty[slot] = true;
	promoteCacheLine(slot);

	//fprintf(stderr, "GPCH: bank %u wrote word %.8X to address %.8X\n", mBankNumber, data, address);

	return true;
}

bool GeneralPurposeCacheHandler::writeHalfWord(uint32_t address, uint32_t data, bool resume, bool debug, int core, int retCh) {
	assert((address & 0x1) == 0);
  if (mBackgroundMemory->readOnly(address))
    throw ReadOnlyException(address);

	uint slot;
	if (!lookupCacheLine(address, slot, resume, false, false)) {
		assert(!resume);
		if (!debug)
			Instrumentation::MemoryBank::writeHalfWord(mBankNumber, address, true, core, retCh);
		if (Arguments::memoryTrace() && !debug)
			MemoryTrace::writeHalfWord(mBankNumber, address);
		return false;
	}

	if (!resume && !debug)
		Instrumentation::MemoryBank::writeHalfWord(mBankNumber, address, false, core, retCh);

	if (Arguments::memoryTrace() && !resume && !debug)
		MemoryTrace::writeHalfWord(mBankNumber, address);

	uint32_t oldData = mData[slot * mLineSize / 4 + (address & mLineMask) / 4];

	if ((address & 0x3) == 0)	// Little endian
		mData[slot * mLineSize / 4 + (address & mLineMask) / 4] = (oldData & 0xFFFF0000UL) | (data & 0x0000FFFFUL);
	else
		mData[slot * mLineSize / 4 + (address & mLineMask) / 4] = (oldData & 0x0000FFFFUL) | (data << 16);

	mLineDirty[slot] = true;
	promoteCacheLine(slot);

	//fprintf(stderr, "GPCH: bank %u wrote half-word %.4X to address %.8X\n", mBankNumber, data & 0xFFFF, address);

	return true;
}

bool GeneralPurposeCacheHandler::writeByte(uint32_t address, uint32_t data, bool resume, bool debug, int core, int retCh) {
  if (mBackgroundMemory->readOnly(address))
    throw ReadOnlyException(address);

	uint slot;
	if (!lookupCacheLine(address, slot, resume, false, false)) {
		assert(!resume);
		if (!debug)
			Instrumentation::MemoryBank::writeByte(mBankNumber, address, true, core, retCh);
		if (Arguments::memoryTrace() && !debug)
			MemoryTrace::writeByte(mBankNumber, address);
		return false;
	}

	if (!resume && !debug)
		Instrumentation::MemoryBank::writeByte(mBankNumber, address, false, core, retCh);

	if (Arguments::memoryTrace() && !resume && !debug)
		MemoryTrace::writeByte(mBankNumber, address);

	uint32_t oldData = mData[slot * mLineSize / 4 + (address & mLineMask) / 4];
	uint32_t selector = address & 0x3UL;

	switch (selector) {	// Little endian
	case 0:	mData[slot * mLineSize / 4 + (address & mLineMask) / 4] = (oldData & 0xFFFFFF00UL) | (data & 0x000000FFUL);				break;
	case 1:	mData[slot * mLineSize / 4 + (address & mLineMask) / 4] = (oldData & 0xFFFF00FFUL) | ((data & 0x000000FFUL) << 8);		break;
	case 2:	mData[slot * mLineSize / 4 + (address & mLineMask) / 4] = (oldData & 0xFF00FFFFUL) | ((data & 0x000000FFUL) << 16);		break;
	case 3:	mData[slot * mLineSize / 4 + (address & mLineMask) / 4] = (oldData & 0x00FFFFFFUL) | ((data & 0x000000FFUL) << 24);		break;
	}

	mLineDirty[slot] = true;
	promoteCacheLine(slot);

	//fprintf(stderr, "GPCH: bank %u wrote byte %.2X to address %.8X\n", mBankNumber, data & 0xFF, address);

	return true;
}

void GeneralPurposeCacheHandler::prepareCacheLine(uint32_t address, uint32_t &writeBackAddress, uint &writeBackCount, uint32_t writeBackData[], uint32_t &fetchAddress, uint &fetchCount) {
	uint slot;
	//assert(!lookupCacheLine(address, slot));

	uint setIndex = (address & mSetMask) >> mSetShift;
	slot = setIndex * mWayCount;

	if (cRandomReplacement) {
		// Select way pseudo-randomly

		slot += mLFSRState % mWayCount;

		// 16-bit Fibonacci LFSR

		uint oldValue = mLFSRState;
		uint newBit = ((oldValue >> 5) & 0x1) ^ ((oldValue >> 3) & 0x1) ^ ((oldValue >> 2) & 0x1) ^ (oldValue & 0x1);
		uint newValue = (oldValue >> 1) | (newBit << 15);
		mLFSRState = newValue;
	} else {
		// Oldest entry always in way with highest index	fprintf(stderr, "GPCH: bank %u wrote word %.8X to address %.8X\n", mBankNumber, data, address);


		slot += mWayCount - 1;
	}

	if (mLineValid[slot]) {
		if (mLineDirty[slot]) {
			Instrumentation::MemoryBank::replaceCacheLine(mBankNumber, true, true);

			writeBackAddress = mAddresses[slot];
			writeBackCount = mLineSize / 4;
			memcpy(writeBackData, &mData[slot * mLineSize / 4], mLineSize);

			//if (mBankNumber >= 4)
			//	fprintf(stderr, "GPCH: bank %u wrote back line at %.8X (%u bytes)\n", mBankNumber, writeBackAddress, writeBackCount * 4);
		} else {
		  Instrumentation::MemoryBank::replaceCacheLine(mBankNumber, true, false);

			writeBackCount = 0;

			//if (mBankNumber >= 4)
			//	fprintf(stderr, "GPCH: bank %u discarded line at %.8X (%u bytes)\n", mBankNumber, mAddresses[slot], mLineSize);
		}
	} else {
	  Instrumentation::MemoryBank::replaceCacheLine(mBankNumber, false, false);

		writeBackCount = 0;

		//if (mBankNumber >= 4)
		//	fprintf(stderr, "GPCH: bank %u prepared empty set slot (%u bytes)\n", mBankNumber, mLineSize);
	}

	fetchAddress = address & ~mLineMask;
	fetchCount = mLineSize / 4;

	mVictimSlot = slot;
}

void GeneralPurposeCacheHandler::replaceCacheLine(uint32_t fetchAddress, uint32_t fetchData[]) {
	memcpy(&mData[mVictimSlot * mLineSize / 4], fetchData, mLineSize);
	mAddresses[mVictimSlot] = fetchAddress;
	mLineValid[mVictimSlot] = true;
	mLineDirty[mVictimSlot] = false;

	//if (mBankNumber >= 4)
	//	fprintf(stderr, "GPCH: bank %u inserted line at %.8X (%u bytes)\n", mBankNumber, fetchAddress, mLineSize);
}

void GeneralPurposeCacheHandler::setBackgroundMemory(SimplifiedOnChipScratchpad *backgroundMemory) {
  mBackgroundMemory = backgroundMemory;
}

void GeneralPurposeCacheHandler::synchronizeData(SimplifiedOnChipScratchpad *backgroundMemory) {
	assert(backgroundMemory != NULL);

	// TODO: call this function when the memory bank is reconfigured.
	// Reconfiguration invalidates cache contents, but doesn't flush the data.
	// This synchronisation will be data accurate but not cycle accurate.

	for (uint setIndex = 0; setIndex < mSetCount; setIndex++) {
		for (uint setSlot = 0; setSlot < mWayCount; setSlot++) {
			uint slot = setIndex * mWayCount + setSlot;
			if (mLineValid[slot] && mLineDirty[slot]) {
				uint address = mAddresses[slot];

				for (uint wordIndex = 0; wordIndex < mLineSize / 4; wordIndex++)
					backgroundMemory->writeWord(address + wordIndex * 4, mData[slot * mLineSize / 4 + wordIndex]);
			}
		}
	}
}
