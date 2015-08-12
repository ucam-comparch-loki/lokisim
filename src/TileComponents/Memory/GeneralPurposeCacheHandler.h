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

class CacheLineBuffer;

class GeneralPurposeCacheHandler : public AbstractMemoryHandler {
private:
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

	const bool cRandomReplacement;// Replace random cache lines (instead of using ideal LRU scheme)


	//---------------------------------------------------------------------------------------------
	// State
	//---------------------------------------------------------------------------------------------

	vector<MemoryTag> mAddresses;	// Cache tags associated with cache lines (full addresses in the model for simplicity reasons)
	vector<bool>      mLineValid;	// Flags indicating whether a particular cache line holds valid data
	vector<bool>      mLineDirty;	// Flags indicating whether a particular cache line is modified

	uint16_t mLFSRState;					// State of LFSR used for random replacement strategy

	uint mSetBits;							// Number of bits used as set index
	uint mSetMask;							// Bit mask to extract set index from address
	uint mSetShift;							// Shift count to align extracted set index

	//---------------------------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------------------------

	bool lookupCacheLine(MemoryAddr address, uint &slot, bool resume, bool read, bool instruction);
	void promoteCacheLine(uint slot);

public:
	GeneralPurposeCacheHandler(uint bankNumber, vector<uint32_t>& data);
	~GeneralPurposeCacheHandler();

  virtual uint32_t readWord(SRAMAddress address);
  virtual uint32_t readHalfWord(SRAMAddress address);
  virtual uint32_t readByte(SRAMAddress address);

  virtual void writeWord(SRAMAddress address, uint32_t data);
  virtual void writeHalfWord(SRAMAddress address, uint32_t data);
  virtual void writeByte(SRAMAddress address, uint32_t data);

  virtual CacheLookup lookupCacheLine(MemoryAddr address) const;
	virtual CacheLookup prepareCacheLine(MemoryAddr address, CacheLineBuffer& lineBuffer, bool isRead, bool isInstruction);
	virtual void replaceCacheLine(CacheLineBuffer& buffer, SRAMAddress position);
	virtual void fillCacheLineBuffer(MemoryAddr address, CacheLineBuffer& buffer);
};

#endif /* GENERALPURPOSECACHEHANDLER_H_ */
