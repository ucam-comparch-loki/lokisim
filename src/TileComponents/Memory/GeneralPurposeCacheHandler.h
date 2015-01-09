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

#include "AbstractMemoryHandler.h"
#include "SimplifiedOnChipScratchpad.h"

class GeneralPurposeCacheHandler : public AbstractMemoryHandler {
private:
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

	bool cRandomReplacement;				// Replace random cache lines (instead of using ideal LRU scheme)

	//---------------------------------------------------------------------------------------------
	// State
	//---------------------------------------------------------------------------------------------

//	uint32_t *mData;						// Data words stored in the cache
	uint32_t *mAddresses;					// Cache tags associated with cache lines (full addresses in the model for simplicity reasons)
	bool *mLineValid;						// Flags indicating whether a particular cache line holds valid data
	bool *mLineDirty;						// Flags indicating whether a particular cache line is modified

	uint16_t mLFSRState;					// State of LFSR used for random replacement strategy

	uint32_t mVictimSlot;					// Slot address of victim cache line about to be replaced

	uint mSetBits;							// Number of bits used as set index
	uint mSetMask;							// Bit mask to extract set index from address
	uint mSetShift;							// Shift count to align extracted set index

  SimplifiedOnChipScratchpad *mBackgroundMemory; // Lowest level cache for debug use

  bool mL2Mode;               // Whether this bank is acting as an L1 or L2 cache.

	//---------------------------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------------------------

	bool lookupCacheLine(MemoryAddr address, uint &slot, bool resume, bool read, bool instruction);
	void promoteCacheLine(uint slot);

public:
	GeneralPurposeCacheHandler(uint bankNumber);
	~GeneralPurposeCacheHandler();

	virtual void activate(const MemoryConfig& config);
	void activateL2(const MemoryConfig& config);

	virtual bool containsAddress(MemoryAddr address);
	virtual bool sameLine(MemoryAddr address1, MemoryAddr address2);

	virtual bool readWord(MemoryAddr address, uint32_t &data, bool instruction, bool resume, bool debug);
	virtual bool readHalfWord(MemoryAddr address, uint32_t &data, bool resume, bool debug);
	virtual bool readByte(MemoryAddr address, uint32_t &data, bool resume, bool debug);

	virtual bool writeWord(MemoryAddr address, uint32_t data, bool resume, bool debug);
	virtual bool writeHalfWord(MemoryAddr address, uint32_t data, bool resume, bool debug);
	virtual bool writeByte(MemoryAddr address, uint32_t data, bool resume, bool debug);

	void prepareCacheLine(MemoryAddr address, MemoryAddr &writeBackAddress, uint &writeBackCount, uint32_t writeBackData[], uint32_t &fetchAddress, uint &fetchCount);
	void replaceCacheLine(MemoryAddr fetchAddress, uint32_t fetchData[]);

	void setBackgroundMemory(SimplifiedOnChipScratchpad *backgroundMemory);

	void synchronizeData(SimplifiedOnChipScratchpad *backgroundMemory);
};

#endif /* GENERALPURPOSECACHEHANDLER_H_ */
