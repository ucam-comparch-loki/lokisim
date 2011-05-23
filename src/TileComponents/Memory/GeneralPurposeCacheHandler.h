//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// General Purpose Cache Mode Handler Module Definition
//-------------------------------------------------------------------------------------------------
// Models the logic and memory active in the general purpose cache mode of a
// memory bank.
//
// The model is used internally by the memory bank model and is not a SystemC
// module by itself.
//-------------------------------------------------------------------------------------------------
// File:       GeneralPurposeCacheHandler.h
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 11/04/2011
//-------------------------------------------------------------------------------------------------

#ifndef GENERALPURPOSECACHEHANDLER_H_
#define GENERALPURPOSECACHEHANDLER_H_

#include "../../Typedefs.h"

class GeneralPurposeCacheHandler {
private:
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

	uint cSetCount;							// Number of sets in general purpose cache mode
	uint cWayCount;							// Number of ways in general purpose cache mode
	uint cLineSize;							// Size of lines (for cache management and data interleaving)

	bool cRandomReplacement;				// Replace random cache lines (instead of using ideal LRU scheme)

	//---------------------------------------------------------------------------------------------
	// State
	//---------------------------------------------------------------------------------------------

	uint32_t *mData;						// Data words stored in the cache
	uint32_t *mAddresses;					// Cache tags associated with cache lines (full addresses in the model for simplicity reasons)
	bool *mLineValid;						// Flags indicating whether a particular cache line holds valid data
	bool *mLineDirty;						// Flags indicating whether a particular cache line is modified

	uint16_t mLFSRState;					// State of LFSR used for random replacement strategy

	uint32_t mVictimSlot;					// Slot address of victim cache line about to be replaced

	uint mSetBits;							// Number of bits used as set index
	uint mSetMask;							// Bit mask to extract set index from address
	uint mSetShift;							// Shift count to align extracted set index

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
	bool lookupCacheLine(uint32_t address, uint &slot);
	void promoteCacheLine(uint slot);

public:
	GeneralPurposeCacheHandler(uint bankNumber);
	~GeneralPurposeCacheHandler();

	void activate(uint groupIndex, uint groupSize);

	bool containsAddress(uint32_t address);

	bool readWord(uint32_t address, uint32_t &data, bool instruction, bool resume, bool debug);
	bool readHalfWord(uint32_t address, uint32_t &data, bool resume, bool debug);
	bool readByte(uint32_t address, uint32_t &data, bool resume, bool debug);

	bool writeWord(uint32_t address, uint32_t data, bool resume, bool debug);
	bool writeHalfWord(uint32_t address, uint32_t data, bool resume, bool debug);
	bool writeByte(uint32_t address, uint32_t data, bool resume, bool debug);

	void prepareCacheLine(uint32_t address, uint32_t &writeBackAddress, uint &writeBackCount, uint32_t writeBackData[], uint32_t &fetchAddress, uint &fetchCount);
	void replaceCacheLine(uint32_t fetchAddress, uint32_t fetchData[]);
};

#endif /* GENERALPURPOSECACHEHANDLER_H_ */
