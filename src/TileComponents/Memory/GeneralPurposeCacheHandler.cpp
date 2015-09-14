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

#include <cassert>
#include "../../Exceptions/ReadOnlyException.h"
#include "../../Exceptions/UnalignedAccessException.h"
#include "../../Utility/Instrumentation/MemoryBank.h"
#include "../../Utility/Trace/MemoryTrace.h"
#include "CacheLineBuffer.h"
#include "GeneralPurposeCacheHandler.h"
#include "SimplifiedOnChipScratchpad.h"



CacheLookup GeneralPurposeCacheHandler::lookupCacheLine(MemoryAddr address) const {
  MemoryTag tag = address & ~mLineMask;
  uint startSlot = getSlot(address);

  CacheLookup info;
  info.Hit = false;

  // Instrumentation for tag check?
  // Careful - this is used for assertions, so a debug parameter would be needed.

  for (uint i = 0; i < mWayCount; i++) {
    if (mLineValid[startSlot + i] && mAddresses[startSlot + i] == tag) {
      info.Hit = true;
      info.SRAMLine = startSlot + i;
    }
  }

  return info;
}

bool GeneralPurposeCacheHandler::lookupCacheLine(MemoryAddr address, uint &slot, bool resume, bool read, bool instruction) {
  MemoryTag tag = address & ~mLineMask;
  bool hit = false;
  slot = getSlot(address);

  for (uint i = 0; i < mWayCount; i++) {
    if (mLineValid[slot + i] && mAddresses[slot + i] == tag) {
      slot = slot + i;

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

GeneralPurposeCacheHandler::GeneralPurposeCacheHandler(uint bankNumber, vector<uint32_t>& data) :
    AbstractMemoryHandler(bankNumber, data),
    cRandomReplacement(MEMORY_CACHE_RANDOM_REPLACEMENT != 0) {

	//-- State ------------------------------------------------------------------------------------

	mWayCount = 1;
	mLineSize = CACHE_LINE_BYTES;
	mSetCount = MEMORY_BANK_SIZE / (mWayCount * mLineSize);

	mAddresses.assign(CACHE_LINES_PER_BANK, 0);
	mLineValid.assign(CACHE_LINES_PER_BANK, false);
	mLineDirty.assign(CACHE_LINES_PER_BANK, false);

	mLFSRState = 0xFFFFU;

  mLineBits = log2Exact(mLineSize);
  mLineMask = (1UL << mLineBits) - 1UL;

  uint groupBits = log2Exact(MEMS_PER_TILE);

  mSetBits = log2Exact(mSetCount);
  mSetShift = groupBits + mLineBits;
  mSetMask = ((1UL << mSetBits) - 1UL) << mSetShift;
}

GeneralPurposeCacheHandler::~GeneralPurposeCacheHandler() {
}


uint32_t GeneralPurposeCacheHandler::readWord(SRAMAddress address) {
  uint cacheLine = getLine(address);
  assert(mLineValid[cacheLine]);

  uint32_t data = AbstractMemoryHandler::readWord(address);

  // Warning: if this moves data around in the array, our SRAMAddress may be
  // invalid.
  promoteCacheLine(cacheLine);
  return data;
}

uint32_t GeneralPurposeCacheHandler::readHalfWord(SRAMAddress address) {
  uint cacheLine = getLine(address);
  assert(mLineValid[cacheLine]);

  uint32_t data = AbstractMemoryHandler::readHalfWord(address);

  // Warning: if this moves data around in the array, our SRAMAddress may be
  // invalid.
  promoteCacheLine(cacheLine);
  return data;
}

uint32_t GeneralPurposeCacheHandler::readByte(SRAMAddress address) {
  uint cacheLine = getLine(address);
  assert(mLineValid[cacheLine]);

  uint32_t data = AbstractMemoryHandler::readByte(address);

  // Warning: if this moves data around in the array, our SRAMAddress may be
  // invalid.
  promoteCacheLine(cacheLine);
  return data;
}


void GeneralPurposeCacheHandler::writeWord(SRAMAddress address, uint32_t data) {
  uint cacheLine = getLine(address);
  assert(mLineValid[cacheLine]);

  AbstractMemoryHandler::writeWord(address, data);

  mLineDirty[cacheLine] = true;
  // Warning: if this moves data around in the array, our SRAMAddress may be
  // invalid.
  promoteCacheLine(cacheLine);
}

void GeneralPurposeCacheHandler::writeHalfWord(SRAMAddress address, uint32_t data) {
  uint cacheLine = getLine(address);
  assert(mLineValid[cacheLine]);

  AbstractMemoryHandler::writeHalfWord(address, data);

  mLineDirty[cacheLine] = true;
  // Warning: if this moves data around in the array, our SRAMAddress may be
  // invalid.
  promoteCacheLine(cacheLine);
}

void GeneralPurposeCacheHandler::writeByte(SRAMAddress address, uint32_t data) {
  uint cacheLine = getLine(address);
  assert(mLineValid[cacheLine]);

  AbstractMemoryHandler::writeByte(address, data);

  mLineDirty[cacheLine] = true;
  // Warning: if this moves data around in the array, our SRAMAddress may be
  // invalid.
  promoteCacheLine(cacheLine);
}

CacheLookup GeneralPurposeCacheHandler::prepareCacheLine(MemoryAddr address, CacheLineBuffer& lineBuffer, bool read, bool instruction) {
  uint slot;
  CacheLookup info;

  info.Hit = lookupCacheLine(address, slot, false, read, instruction);

  if (!info.Hit) {

    slot = getSlot(address);

    if (cRandomReplacement) {
      // Select way pseudo-randomly
      slot += mLFSRState % mWayCount;

      // 16-bit Fibonacci LFSR
      uint oldValue = mLFSRState;
      uint newBit = ((oldValue >> 5) & 0x1) ^ ((oldValue >> 3) & 0x1) ^ ((oldValue >> 2) & 0x1) ^ (oldValue & 0x1);
      uint newValue = (oldValue >> 1) | (newBit << 15);
      mLFSRState = newValue;
    }
    else {
      // Oldest entry always in way with highest index
      slot += mWayCount - 1;
    }

    // Line only needs to be flushed if it is both valid and dirty.
    if (mLineValid[slot] && mLineDirty[slot]) {
      info.FlushAddress = mAddresses[slot];
      info.FlushRequired = true;
      lineBuffer.fill(info.FlushAddress, &mData[slot * mLineSize / 4]);
    }
    else
      info.FlushRequired = false;

    Instrumentation::MemoryBank::replaceCacheLine(mBankNumber, mLineValid[slot], mLineDirty[slot]);
  }

  info.SRAMLine = slot;

  return info;
}

void GeneralPurposeCacheHandler::replaceCacheLine(CacheLineBuffer& buffer, SRAMAddress position, bool dirtyLine) {
  uint cacheLine = getLine(position);

  buffer.flush(&mData[position / 4]);
  mAddresses[cacheLine] = buffer.getTag();
  mLineValid[cacheLine] = true;
  mLineDirty[cacheLine] = dirtyLine;
}

void GeneralPurposeCacheHandler::fillCacheLineBuffer(MemoryAddr address, CacheLineBuffer& buffer) {
  uint slot;
  // FIXME: tag check causes instrumentation, but this is just for debug.
  bool cacheHit = lookupCacheLine(address, slot, false, true, false);
  assert(cacheHit);

  buffer.fill(address, &mData[slot * mLineSize / 4]);
}

void GeneralPurposeCacheHandler::invalidate(SRAMAddress address) {
  uint cacheLine = getLine(address);
  mLineValid[cacheLine] = false;
}
