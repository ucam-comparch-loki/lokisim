//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Memory Bank Definition
//-------------------------------------------------------------------------------------------------
// Defines a cache memory bank.
//-------------------------------------------------------------------------------------------------
// File:       SharedL1CacheBank.h
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 13/01/2011
//-------------------------------------------------------------------------------------------------

#ifndef SHAREDL1CACHEBANK_HPP_
#define SHAREDL1CACHEBANK_HPP_

#include "../../Component.h"

class SharedL1CacheBank : public Component {
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

private:

	uint					cBankNumber;			// Number of this memory bank

	uint					cMemoryBanks;			// Number of memory banks
	uint					cMemoryBankSize;		// Size of memory bank
	uint					cSetCount;				// Number of sets in the memory bank
	uint					cAssociativity;			// Associativity level
	uint					cCacheLineSize;			// Size of cache lines

	uint					cAddressTagBits;		// Number of high order bits used to generate cache tags
	uint					cSetSelectionBits;		// Number of intermediate bits used to select a cache set
	uint					cBankSelectionBits;		// Number of intermediate bits used to select memory bank
	uint					cCacheLineBits;			// Number of low order bits used to select position in cache line (lowest two bits are ignored)

	bool					cSequentialSearch;		// Search cache tags sequentially
	bool					cRandomReplacement;		// Replace random cache lines (instead of using LRU scheme)

	//---------------------------------------------------------------------------------------------
	// Ports
	//---------------------------------------------------------------------------------------------

public:

	// Clock

	sc_in<bool>				iClock;					// Clock

	// Ports to crossbar switch

	sc_in<uint32_t>			iAddress;				// Address input from crossbar switch
	sc_in<uint64_t>			iWriteData;				// Data word input from crossbar switch
	sc_in<uint8_t>			iByteMask;				// Byte mask input from crossbar switch
	sc_in<bool>				iReadEnable;			// Read enable signal input from crossbar switch
	sc_in<bool>				iWriteEnable;			// Write enable signal input from crossbar switch

	sc_out<uint64_t>		oReadData;				// Data word output to crossbar switch
	sc_out<bool>			oAcknowledge;			// Acknowledgement signal output to crossbar switch

	// Ports to background memory

	sc_out<uint32_t>		oAddress;				// Address output to background memory
	sc_out<uint64_t>		oWriteData;				// Data word output to background memory
	sc_out<bool>			oReadEnable;			// Read enable signal output to background memory
	sc_out<bool>			oWriteEnable;			// Write enable signal output to background memory

	sc_in<uint64_t>			iReadData;				// Data word input from background memory
	sc_in<bool>				iAcknowledge;			// Acknowledgement signal input from background memory

	//---------------------------------------------------------------------------------------------
	// Utility definitions
	//---------------------------------------------------------------------------------------------

private:

	enum FSMState {
		STATE_READY,								// Ready to service memory request
		STATE_READ_LOOKUP,							// Comparing cache tags due to pending read request
		STATE_WRITE_LOOKUP,							// Comparing cache tags due to pending write request
		STATE_REPLACE_QUERY,						// Cache miss occurred
		STATE_REPLACE_WRITE_BACK,					// Cache line to replace got selected and is dirty
		STATE_REPLACE_FETCH_REQUEST,				// Requesting data words of new cache line from the background memory
		STATE_REPLACE_FETCH_STORE					// Waiting for the background memory to send the requested data words and store them into the cache memory
	};

	//---------------------------------------------------------------------------------------------
	// Signals
	//---------------------------------------------------------------------------------------------

private:

	// Input helper signals

	sc_signal<uint32_t>		sInputAddressTag;		// Address tag based on iAddress
	sc_signal<uint32_t>		sInputSetAddress;		// Set address based on iAddress
	sc_signal<uint32_t>		sInputBankAddress;		// Bank address based on iAddress
	sc_signal<uint32_t>		sInputLineAddress;		// Cache line address based on iAddress (2 lowest bits of logical address omitted!)

	// FSM registers

	sc_signal<FSMState>		rCurrentState;			// Current state of the controller FSM
	sc_signal<uint8_t>		rTagCursor;				// Index of cache tag currently searched
	sc_signal<uint32_t>		rSetIndex;				// Index of cache line in set currently processed
	sc_signal<uint8_t>		rLineCursor;			// Index of word in cache line currently processed
	sc_signal<uint32_t>		rMemoryAddress;			// Index of address in background memory currently processed
	sc_signal<uint16_t>		rLFSR;					// Linear feedback shift register for random replacement strategy

	// FSM signals

	sc_signal<FSMState>		sNextState;				// Next state of the controller FSM
	sc_signal<uint8_t>		sTagCursor;				// New index of cache tag currently searched
	sc_signal<uint32_t>		sSetIndex;				// New index of cache line in set currently processed
	sc_signal<uint8_t>		sLineCursor;			// New index of word in cache line currently processed
	sc_signal<uint32_t>		sMemoryAddress;			// New address in background memory currently processed
	sc_signal<uint16_t>		sLFSR;					// New value of linear feedback shift register for random replacement strategy

	// Cache memory control signals

	sc_signal<uint32_t>		sCacheSetAddress;		// Address of cache set to access
	sc_signal<uint32_t>		sCacheSetIndex;			// Entry within cache set to access (action depends on sequential or parallel mode)
	sc_signal<uint32_t>		sCacheLineAddress;		// Address of word within cache line (2 lowest bits of logical address omitted!)

	sc_signal<uint32_t>		sCacheTag;				// Cache tag to compare

	sc_signal<uint64_t>		sCacheData;				// Data to write to cache
	sc_signal<uint8_t>		sCacheByteMask;			// Byte mask applied to data to write to cache (update operation only)

	sc_signal<bool>			sCacheLookupEnable;		// Indicates that the cache memory shall perform a lookup operation
	sc_signal<bool>			sCacheUpdateEnable;		// Indicates that the cache memory shall perform an update operation

	sc_signal<bool>			sCacheQueryEnable;		// Indicates that the cache memory shall perform a replacement query retrieving cache set index, cache tag and dirty flag
	sc_signal<bool>			sCacheStoreEnable;		// Indicates that the cache memory shall perform a metadata update for a cache line being replaced (can be asserted in parallel to sCacheWriteEnable)
	sc_signal<bool>			sCacheReadEnable;		// Indicates that the cache memory shall perform an unconditional read
	sc_signal<bool>			sCacheWriteEnable;		// Indicates that the cache memory shall perform an unconditional write

	// Cache memory output registers

	sc_signal<bool>			rCacheHit;				// Lookup or update operation successful
	sc_signal<uint64_t>		rCacheData;				// Data word retrieved from cache

	sc_signal<uint32_t>		rCacheSetIndex;			// Cache set index containing cache line to be replaced
	sc_signal<uint32_t>		rCacheTag;				// Cache tag retrieved from cache
	sc_signal<bool>			rCacheDirty;			// Selected cache line is dirty

	// Cache memory data cells

	sc_signal<bool>			*rCellsValidFlags;		// Cache valid flags (theoretically cAssociativity individual memories)
	sc_signal<bool>			*rCellsDirtyFlags;		// Cache dirty flags (theoretically cAssociativity individual memories)
	sc_signal<uint8_t>		*rCellsLRUCounters;		// Cache LRU counters (theoretically cAssociativity individual memories)
	sc_signal<uint32_t>		*rCellsCacheTags;		// Cache tag array (theoretically cAssociativity individual memories)
	sc_signal<uint64_t>		*rCellsCacheData;		// Cache data array (theoretically cAssociativity individual memories)

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods
	//---------------------------------------------------------------------------------------------

	void debugOutputMessage(const char* message, long long arg1, long long arg2, long long arg3);

	//---------------------------------------------------------------------------------------------
	// Processes
	//---------------------------------------------------------------------------------------------

private:
	// Utility signal process

	void processUtilitySignals();

	// FSM register logic sensitive to negative clock edge

	void processFSMRegisters();

	// Combinational FSM logic

	void processFSMCombinational();
	void subProcessStateReadLookup();
	void subProcessStateWriteLookup();
	void subProcessStateReplaceQuery();
	void subProcessStateReplaceWriteBack();
	void subProcessStateReplaceFetchRequest();
	void subProcessStateReplaceFetchStore();
	void subProcessInitiateRequest();
	void subProcessEnableLFSR();

	// Cache memory logic sensitive to negative clock edge

	void processCacheMemory();

	//---------------------------------------------------------------------------------------------
	// Constructors / Destructors
	//---------------------------------------------------------------------------------------------

public:

	SC_HAS_PROCESS(SharedL1CacheBank);
	SharedL1CacheBank(sc_module_name name, ComponentID id, uint bankNumber, uint memoryBanks, uint cacheSetCount, uint associativity, uint cacheLineSize, bool sequentialSearch, bool randomReplacement);
	virtual ~SharedL1CacheBank();

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods - not part of simulated logic
	//---------------------------------------------------------------------------------------------

public:

	// Update the memory contents

	virtual bool setWord(uint32_t address, const uint64_t data);

	// Retrieve the memory contents

	virtual bool getWord(uint32_t address, uint64_t &data);

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods inherited from Component - not part of simulated logic
	//---------------------------------------------------------------------------------------------

public:

	// The area of this component in square micrometres

	virtual double area() const;

	// The energy consumed by this component in picojoules

	virtual double energy() const;
};

#endif /* SHAREDL1CACHEBANK_HPP_ */
