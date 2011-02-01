//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Memory Bank Implementation
//-------------------------------------------------------------------------------------------------
// Implements a cache memory bank.
//-------------------------------------------------------------------------------------------------
// File:       SharedL1CacheBank.cpp
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 13/01/2011
//-------------------------------------------------------------------------------------------------

#include "../../Component.h"
#include "SharedL1CacheBank.h"

//-------------------------------------------------------------------------------------------------
// Processes
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Utility signal process
//-------------------------------------------------------------------------------------------------
// Description:
//
// Splits the memory address sent by the crossbar switch into its components
//-------------------------------------------------------------------------------------------------
// Input assertions:
//
// - iAddress contains word aligned memory address to access
//-------------------------------------------------------------------------------------------------
// Output assertions:
//
// - sInput* contain split memory address
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::processUtilitySignals() {
	uint32_t address = iAddress.read();

	if ((address & 0x3) != 0)
		cerr << "Error: Unaligned memory address detected at cache bank" << endl;

	sInputAddressTag.write(address & ~((1UL << cCacheLineBits) - 1));
	sInputSetAddress.write((address >> (cBankSelectionBits + cCacheLineBits)) & ((1UL << cSetSelectionBits) - 1));
	sInputBankAddress.write((address >> cCacheLineBits) & ((1UL << cBankSelectionBits) - 1));
	sInputLineAddress.write((address & ((1UL << cCacheLineBits) - 1)) >> 2);
}

//-------------------------------------------------------------------------------------------------
// FSM register logic sensitive to negative clock edge
//-------------------------------------------------------------------------------------------------
// Description:
//
// FSM register triggered by negative clock edge of iClock signal
//-------------------------------------------------------------------------------------------------
// Input assertions:
//
// - sNextState contains new state of FSM
// - sTagCursor contains new index of cache tag currently searched (sequential search mode only)
// - sSetIndex contains new index of cache line in set currently processed
// - sLineCursor contains new index of word in cache line currently processed
// - sMemoryAddress contains new address in background memory currently processed
// - sLFSR contains new state of LFSR (random replacement strategy only)
//-------------------------------------------------------------------------------------------------
// Output assertions:
//
// - Corresponding FSM registers updated
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::processFSMRegisters() {
	rCurrentState.write(sNextState.read());

	if (cSequentialSearch)
		rTagCursor.write(sTagCursor.read());

	rSetIndex.write(sSetIndex.read());
	rLineCursor.write(sLineCursor.read());
	rMemoryAddress.write(sMemoryAddress.read());

	if (cRandomReplacement)
		rLFSR.write(sLFSR.read());
}

//-------------------------------------------------------------------------------------------------
// Combinational FSM logic
//-------------------------------------------------------------------------------------------------
// Description:
//
// Combinational signal assignments of FSM
//-------------------------------------------------------------------------------------------------
// Input assertions:
//
// - rCurrentState contains current state of FSM
//-------------------------------------------------------------------------------------------------
// Output assertions:
//
// - sNextState contains new state of FSM
// - All output ports and signals set to default values unless modified by state handler
// - All register clock signals disabled unless register content modified by state handler
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::processFSMCombinational() {
	// Break dependency chains on port values to make the following code more compact

	oReadData.write(0);					// Should really be don't care
	oAcknowledge.write(false);

	oAddress.write(0);					// Should really be don't care
	oWriteData.write(0);				// Should really be don't care
	oReadEnable.write(false);
	oWriteEnable.write(false);

	// Break dependency chains on cache memory signal values to make following code more compact

	sCacheSetAddress.write(0);			// Should really be don't care
	sCacheSetIndex.write(0);			// Should really be don't care
	sCacheLineAddress.write(0);			// Should really be don't care

	sCacheTag.write(0);					// Should really be don't care

	sCacheData.write(0);				// Should really be don't care
	sCacheByteMask.write(0);			// Should really be don't care

	sCacheLookupEnable.write(false);
	sCacheUpdateEnable.write(false);

	sCacheQueryEnable.write(false);
	sCacheStoreEnable.write(false);
	sCacheReadEnable.write(false);
	sCacheWriteEnable.write(false);

	// Implicitly generate state based clock enable signals by holding register values whenever they are not changed (saves both area and energy)

	if (cSequentialSearch)
		sTagCursor.write(rTagCursor.read());

	sLineCursor.write(rLineCursor.read());
	sSetIndex.write(rSetIndex.read());
	sMemoryAddress.write(rMemoryAddress.read());

	if (cRandomReplacement)
		sLFSR.write(rLFSR.read());

	// Steer port and signal values based on current FSM state and input port values

	switch (rCurrentState.read()) {
	case STATE_READY:
		// Ready to service memory request

		subProcessInitiateRequest();
		break;

	case STATE_READ_LOOKUP:
		// Comparing cache tags due to pending read request

		subProcessStateReadLookup();
		break;

	case STATE_WRITE_LOOKUP:
		// Comparing cache tags due to pending write request

		subProcessStateWriteLookup();
		break;

	case STATE_REPLACE_QUERY:
		// Cache miss occurred

		subProcessStateReplaceQuery();
		break;

	case STATE_REPLACE_WRITE_BACK:
		// Cache line to replace got selected and is dirty

		subProcessStateReplaceWriteBack();
		break;

	case STATE_REPLACE_FETCH_REQUEST:
		// Requesting data words of new cache line from the background memory

		subProcessStateReplaceFetchRequest();
		break;

	case STATE_REPLACE_FETCH_STORE:
		// Waiting for the background memory to send the requested data words and store them into the cache memory

		subProcessStateReplaceFetchStore();
		break;
	}
}

//-------------------------------------------------------------------------------------------------
// FSM state handler STATE_READ_LOOKUP
//-------------------------------------------------------------------------------------------------
// Description:
//
// This state is entered following a cache lookup due to a read request. It needs to check whether
// the cache line was hit in the cache. In this case it return the data to the crossbar. If the
// access resulted in a miss it either has to continue search in case of sequential search mode or
// eventually initiate the replacement of one of the cache lines in the set.
//-------------------------------------------------------------------------------------------------
// Input assertions:
//
// - iAddress is held constant indicating the word aligned address to access
// - rCacheHit indicates whether the previous lookup was successful
// - rCacheData contains the data read from the cache in this case
// - rTagCursor contains the set index looked up in the previous clock cycle (sequential search
//   mode only)
// - rLFSR contains a valid (i.e. non-zero) LFSR state (random replacement strategy only)
//-------------------------------------------------------------------------------------------------
// Output assertions:
//
// Cache hit:
//
// - oReadData is set to the data read from the cache in case of a hit
// - oAcknowledge indicates this fact
// - New request is initiated by subProcessInitiateRequest()
//
// Non-ultimate cache miss in sequential search mode:
//
// - sCacheSetAddress points at the cache set to access
// - sCacheSetIndex points at the set entry to access
// - sCacheLineAddress points at the word in the cache line to access
// - sCacheTag points at the beginning of the cache line in the background memory
// - sCacheLookupEnable is asserted
// - sTagCursor is updated to point at the set entry to access in the current clock cycle
// - Next FSM state: STATE_READ_LOOKUP
//
// Ultimate cache miss:
//
// - sCacheSetAddress points at the cache set to access
// - sCacheSetIndex points at the set index to replace (random replacement strategy only)
// - sCacheQueryEnable is asserted
// - LFSR is being updated
// - Next FSM state: STATE_REPLACE_QUERY
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::subProcessStateReadLookup() {
	// Comparing cache tags due to pending read request

	if (rCacheHit.read()) {
		// Cache line was found in cache

		oReadData.write(rCacheData.read());
		oAcknowledge.write(true);

		// Proceed with next request or become idle

		subProcessInitiateRequest();
	} else if (cSequentialSearch && rTagCursor.read() != cAssociativity - 1) {
		// Cache line was not found in cache yet - search needs to proceed

		sCacheSetAddress.write(sInputSetAddress.read());
		sCacheLineAddress.write(sInputLineAddress.read());
		sCacheTag.write(sInputAddressTag.read());

		sCacheSetIndex.write(rTagCursor.read() + 1);
		sTagCursor.write(rTagCursor.read() + 1);

		sCacheLookupEnable.write(true);

		sNextState.write(STATE_READ_LOOKUP);
	} else {
		// Cache line was not found in cache - query cache for cache line to replace

		sCacheSetAddress.write(sInputSetAddress.read());

		if (cRandomReplacement) {
			sCacheSetIndex.write(rLFSR.read() % cAssociativity);
			subProcessEnableLFSR();
		}

		sCacheQueryEnable.write(true);

		sNextState.write(STATE_REPLACE_QUERY);
	}
}

//-------------------------------------------------------------------------------------------------
// FSM state handler STATE_WRITE_LOOKUP
//-------------------------------------------------------------------------------------------------
// Description:
//
// This state is entered following a cache lookup due to a write request. It needs to check
// whether the cache line was hit in the cache. In this case it return the data to the crossbar.
// If the access resulted in a miss it either has to continue search in case of sequential search
// mode or eventually initiate the replacement of one of the cache lines in the set.
//-------------------------------------------------------------------------------------------------
// Input assertions:
//
// - iAddress is held constant indicating the word aligned address to access
// - iWriteData is held constant containing data to write in case of a write request
// - iByteMask is held constant indicating which bytes to write in case of a write request
// - rCacheHit indicates whether the previous lookup / update was successful
// - rTagCursor contains the set index looked up in the previous clock cycle (sequential search
//   mode only)
// - rLFSR contains a valid (i.e. non-zero) LFSR state (random replacement strategy only)
//-------------------------------------------------------------------------------------------------
// Output assertions:
//
// Cache hit:
//
// - oAcknowledge indicates that the data got written
// - New request is initiated by subProcessInitiateRequest()
//
// Non-ultimate cache miss in sequential search mode:
//
// - sCacheSetAddress points at the cache set to access
// - sCacheSetIndex points at the set entry to access
// - sCacheLineAddress points at the word in the cache line to access
// - sCacheTag points at the beginning of the cache line in the background memory
// - sCacheData contains the data word to write
// - sCacheByteMask indicates which bytes to write
// - sCacheUpdateEnable is asserted
// - sTagCursor is updated to point at the set entry to access in the current clock cycle
// - Next FSM state: STATE_WRITE_LOOKUP
//
// Ultimate cache miss:
//
// - sCacheSetAddress points at the cache set to access
// - sCacheSetIndex points at the set index to replace (random replacement strategy only)
// - sCacheQueryEnable is asserted
// - LFSR is being updated
// - Next FSM state: STATE_REPLACE_QUERY
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::subProcessStateWriteLookup() {
	// Comparing cache tags due to pending read request

	if (rCacheHit.read()) {
		// Cache line was found in cache

		oAcknowledge.write(true);

		// Proceed with next request or become idle

		subProcessInitiateRequest();
	} else if (cSequentialSearch && rTagCursor.read() != cAssociativity - 1) {
		// Cache line was not found in cache yet - search needs to proceed

		sCacheSetAddress.write(sInputSetAddress.read());
		sCacheLineAddress.write(sInputLineAddress.read());
		sCacheTag.write(sInputAddressTag.read());

		sCacheData.write(iWriteData.read());
		sCacheByteMask.write(iByteMask.read());

		sCacheSetIndex.write(rTagCursor.read() + 1);
		sTagCursor.write(rTagCursor.read() + 1);

		sCacheUpdateEnable.write(true);

		sNextState.write(STATE_WRITE_LOOKUP);
	} else {
		// Cache line was not found in cache - query cache for cache line to replace

		sCacheSetAddress.write(sInputSetAddress.read());

		if (cRandomReplacement) {
			sCacheSetIndex.write(rLFSR.read() % cAssociativity);
			subProcessEnableLFSR();
		}

		sCacheQueryEnable.write(true);

		sNextState.write(STATE_REPLACE_QUERY);
	}
}

//-------------------------------------------------------------------------------------------------
// FSM state handler STATE_REPLACE_QUERY
//-------------------------------------------------------------------------------------------------
// Description:
//
// This state is entered following a cache miss. It needs to check whether the cache line to be
// replaced is dirty. If it is the write back sequence must be initiated. Otherwise the cache line
// can be overwritten directly.
//-------------------------------------------------------------------------------------------------
// Input assertions:
//
// - iAddress is held constant indicating the word aligned address to access
// - rCacheDirty indicates whether the cache line selected for replacement is dirty
// - rCacheSetIndex points at the set entry selected for replacement
// - rCacheTag points at the address of the selected cache line in the background memory
//-------------------------------------------------------------------------------------------------
// Output assertions:
//
// Cache line is dirty:
//
// - sCacheSetAddress points at the cache set to access
// - sCacheSetIndex points at the set entry to replace
// - sCacheLineAddress points at the first word in the cache line to replace
// - sCacheReadEnable is asserted
// - sMemoryAddress is set to the address of the selected cache line in the background memory
//   (buffering of cache memory output needed)
// - sSetIndex is set to the set index selected for replacement (buffering of cache memory
//   output needed)
// - sLineCursor is initialised to point at the first word in the cache line to replace
// - Next FSM state: STATE_REPLACE_WRITE_BACK
//
// Cache line is clean:
//
// - oAddress points at the first data address in the background memory to read
// - oReadEnable is asserted
// - sLineCursor is initialised to point at the first word in the cache line to replace
// - sSetIndex is set to the set index selected for replacement (buffering of cache memory
//   output needed)
// - Next FSM state: STATE_REPLACE_FETCH_REQUEST
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::subProcessStateReplaceQuery() {
	// Cache miss occurred

	if (rCacheDirty.read()) {
		// Cache line needs to be written back to background memory

		sCacheSetAddress.write(sInputSetAddress.read());
		sCacheSetIndex.write(rCacheSetIndex.read());
		sCacheLineAddress.write(0);

		sCacheReadEnable.write(true);

		sSetIndex.write(rCacheSetIndex.read());
		sMemoryAddress.write(rCacheTag.read());
		sLineCursor.write(0);

		sNextState.write(STATE_REPLACE_WRITE_BACK);
	} else {
		// Cache line can be discarded - start to fetch new data directly

		oAddress.write(sInputAddressTag.read());
		oReadEnable.write(true);

		sSetIndex.write(rCacheSetIndex.read());
		sLineCursor.write(0);

		sNextState.write(STATE_REPLACE_FETCH_REQUEST);
	}
}

//-------------------------------------------------------------------------------------------------
// FSM state handler STATE_REPLACE_WRITE_BACK
//-------------------------------------------------------------------------------------------------
// Description:
//
// This state is entered following the selection of a dirty cache line for replacement. It needs
// to send all words contained in the cache line to the background memory.
//-------------------------------------------------------------------------------------------------
// Input assertions:
//
// - iAddress is held constant indicating the word aligned address to access
// - rMemoryAddress points at the address of the selected cache line in the background memory
//   (and is being held by clock enable logic)
// - rSetIndex points at the set entry selected for replacement
//   (and is being held by clock enable logic)
// - rLineCursor points at the word in the cache line read during the previous clock cycle
//   (i.e. the word that is now ready to be sent to the background memory)
// - rCacheData contains the data word read from the cache during the previous clock cycle
//-------------------------------------------------------------------------------------------------
// Output assertions:
//
// Write back not yet completed:
//
// - oAddress is set to the address of the data word being written back to the background memory
// - oWriteData is set to data word being written back to the background memory
// - oWriteEnable is asserted
// - sCacheSetAddress points at the cache set to access
// - sCacheSetIndex points at the set entry to replace
// - sCacheLineAddress points at the word in the cache line to read
// - sCacheReadEnable is asserted
// - sLineCursor is updated to point at the word in the cache line being read
// - Next FSM state: STATE_REPLACE_WRITE_BACK
//
// Write back not yet completed but last word read from cache memory:
//
// - oAddress is set to the address of the data word being written back to the background memory
// - oWriteData is set to data word being written back to the background memory
// - oWriteEnable is asserted
// - sLineCursor is updated to point behind the cache line being replaced
// - Next FSM state: STATE_REPLACE_WRITE_BACK
//
// Write back completed:
//
// - oAddress points at the first data address in the background memory to read
// - oReadEnable is asserted
// - sLineCursor is reset to point at the first word in the cache line to replace
// - Next FSM state: STATE_REPLACE_FETCH_REQUEST
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::subProcessStateReplaceWriteBack() {
	// Cache line to replace got selected and is dirty

	if (rLineCursor.read() < cCacheLineSize / 4 - 1) {
		// Data word needs to be written back to background memory

		oAddress.write(rMemoryAddress.read() + rLineCursor.read() * 4);
		oWriteData.write(rCacheData.read());
		oWriteEnable.write(true);

		sCacheSetAddress.write(sInputSetAddress.read());
		sCacheSetIndex.write(rCacheSetIndex.read());
		sCacheLineAddress.write(rLineCursor.read() + 1);

		sCacheReadEnable.write(true);

		sLineCursor.write(rLineCursor.read() + 1);

		sNextState.write(STATE_REPLACE_WRITE_BACK);
	} else if (rLineCursor.read() == cCacheLineSize / 4 - 1) {
		// Last data word needs to be written back to background memory

		oAddress.write(rMemoryAddress.read() + rLineCursor.read() * 4);
		oWriteData.write(rCacheData.read());
		oWriteEnable.write(true);

		sLineCursor.write(rLineCursor.read() + 1);

		sNextState.write(STATE_REPLACE_WRITE_BACK);
	} else {
		// Cache line written back - initiate fetch of new cache line

		oAddress.write(sInputAddressTag.read());
		oReadEnable.write(true);

		sLineCursor.write(0);

		sNextState.write(STATE_REPLACE_FETCH_REQUEST);
	}
}

//-------------------------------------------------------------------------------------------------
// FSM state handler STATE_REPLACE_FETCH_REQUEST
//-------------------------------------------------------------------------------------------------
// Description:
//
// This state is entered following the selection of a clean cache line for replacement or after
// writing a dirty cache line back to the background memory. It needs to request all words
// contained in the new cache line from the background memory.
//
// NOTE: It is assumed that the first data word from the background memory arrives after all read
// commands were sent. If the delay is to short for that to be guaranteed, this handler needs to
// be adjusted.
//-------------------------------------------------------------------------------------------------
// Input assertions:
//
// - iAddress is held constant indicating the word aligned address to access
// - rSetIndex points at the set entry selected for replacement
//   (and is being held by clock enable logic)
// - rLineCursor points at the word in the cache line requested during the previous clock cycle
//-------------------------------------------------------------------------------------------------
// Output assertions:
//
// Fetch command sequence not yet completed:
//
// - oAddress is set to the address of the data word being requested from the background memory
// - oReadEnable is asserted
// - sLineCursor is updated to point at the word in the cache line being requested
// - Next FSM state: STATE_REPLACE_FETCH_REQUEST
//
// Last fetch command being sent:
//
// - oAddress is set to the address of the data word being requested from the background memory
// - oReadEnable is asserted
// - Next FSM state: STATE_REPLACE_FETCH_STORE
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::subProcessStateReplaceFetchRequest() {
	// Request data words of new cache line from the background memory

	if (iAcknowledge.read())
		cerr << "Error: Background memory delay is too short" << endl;

	if (rLineCursor.read() < cCacheLineSize / 4 - 1) {
		// Fetch command sequence not yet completed

		oAddress.write(sInputAddressTag.read() + (rLineCursor.read() + 1) * 4);
		oReadEnable.write(true);

		sLineCursor.write(rLineCursor.read() + 1);

		sNextState.write(STATE_REPLACE_FETCH_REQUEST);
	} else {
		// Last fetch command

		oAddress.write(sInputAddressTag.read() + (rLineCursor.read() + 1) * 4);
		oReadEnable.write(true);

		sNextState.write(STATE_REPLACE_FETCH_STORE);
	}
}

//-------------------------------------------------------------------------------------------------
// FSM state handler STATE_REPLACE_FETCH_STORE
//-------------------------------------------------------------------------------------------------
// Description:
//
// This state is entered following the selection of a clean cache line for replacement or after
// writing a dirty cache line back to the background memory. It needs to fetch all words contained
// in the new cache line from the background memory and update the cache metadata.
//
// NOTE: It is assumed that the first data word from the background memory arrives after all read
// commands were sent. If the delay is to short for that to be guaranteed, this handler needs to
// be adjusted.
//-------------------------------------------------------------------------------------------------
// Input assertions:
//
// - iAddress is held constant indicating the word aligned address to access
// - iReadData might contain data sent from the background memory
// - iAcknowledge indicates whether the data is valid (and must be consumed)
// - rSetIndex points at the set entry selected for replacement
//   (and is being held by clock enable logic)
// - rLineCursor points at the word in the cache line arriving next from the background memory
//   (and is being held by clock enable logic if it is not being modified)
//-------------------------------------------------------------------------------------------------
// Output assertions:
//
// Store sequence not yet completed and no data available:
//
// - Next FSM state: STATE_REPLACE_FETCH_STORE
//
// Data word available:
//
// - sCacheSetAddress points to the set containing the cache line being written
// - sCacheSetIndex points at the set entry being written
// - sCacheLineAddress points at the word in the cache line being written
// - sCacheData is set to the data word retrieved from the background memory
// - sCacheWriteEnable is asserted
// - sCacheTag contains the address tag being written to the cache memory (first data word only)
// - sCacheStoreEnable is asserted (first data word only)
// - sLineCursor is updated to point at the next word expected from the background memory
// - Next FSM state: STATE_REPLACE_FETCH_STORE for cache line sizes larger than a single word,
//   re-initiate request otherwise
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::subProcessStateReplaceFetchStore() {
	// Wait for the background memory to send the requested data words and store them into the cache memory

	if (iAcknowledge.read()) {
		sCacheSetAddress.write(sInputSetAddress);
		sCacheSetIndex.write(rSetIndex.read());
		sCacheLineAddress.write(rLineCursor.read());
		sCacheData.write(iReadData.read());

		sCacheWriteEnable.write(true);

		if (rLineCursor.read() == 0) {
			// First data word available
			sCacheTag.write(sInputAddressTag);
			sCacheStoreEnable.write(true);
		}

		sLineCursor.write(rLineCursor.read() + 1);

		if (rLineCursor.read() < cCacheLineSize / 4 - 1) {
			sNextState.write(STATE_REPLACE_FETCH_STORE);
		} else {
			subProcessInitiateRequest();
		}
	} else {
		// Store sequence not yet completed and no data available

		sNextState.write(STATE_REPLACE_FETCH_STORE);
	}
}

//-------------------------------------------------------------------------------------------------
// FSM request initiator
//-------------------------------------------------------------------------------------------------
// Description:
//
// This handler serves as the state handler for the STATE_READY state and is also called by other
// state handlers to chain requests within a single clock cycle. It needs to check whether a read
// or write request is pending and initiate it or stay / become idle otherwise.
//-------------------------------------------------------------------------------------------------
// Input assertions:
//
// - iAddress is held constant indicating the word aligned address to access
// - iWriteData is held constant containing data to write in case of a write request
// - iByteMask is held constant indicating which bytes to write in case of a write request
// - iReadEnable indicates a pending read request
// - iWriteEnable indicates a pending write request
//-------------------------------------------------------------------------------------------------
// Output assertions:
//
// - Input signals are forwarded to cache memory to perform lookup
//
// Read request pending:
//
// - sCacheLookupEnable asserted
// - Next FSM state: STATE_READ_LOOKUP
//
// Write request pending:
//
// - sCacheUpdateEnable asserted
// - Next FSM state: STATE_WRITE_LOOKUP
//
// No request pending:
//
// - Next FSM state: STATE_READY
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::subProcessInitiateRequest() {
	if (iReadEnable.read()) {
		// Initiate read request

		if (iWriteEnable.read())
			cerr << "Error: Read and Write Enable signals asserted together" << endl;

		if (sInputBankAddress.read() != cBankNumber)
			cerr << "Error: Read access routed to wrong bank" << endl;

		sCacheSetAddress.write(sInputSetAddress.read());
		sCacheLineAddress.write(sInputLineAddress.read());
		sCacheTag.write(sInputAddressTag.read());

		if (cSequentialSearch) {
			sCacheSetIndex.write(0);
			sTagCursor.write(0);
		}

		sCacheLookupEnable.write(true);

		sNextState.write(STATE_READ_LOOKUP);
	} else if (iWriteEnable.read()) {
		// Initiate write request

		if (iReadEnable.read())
			cerr << "Error: Read and Write Enable signals asserted together" << endl;

		if (sInputBankAddress.read() != cBankNumber)
			cerr << "Error: Write access routed to wrong bank" << endl;

		sCacheSetAddress.write(sInputSetAddress.read());
		sCacheLineAddress.write(sInputLineAddress.read());
		sCacheTag.write(sInputAddressTag.read());

		sCacheData.write(iWriteData.read());
		sCacheByteMask.write(iByteMask.read());

		if (cSequentialSearch) {
			sCacheSetIndex.write(0);
			sTagCursor.write(0);
		}

		sCacheUpdateEnable.write(true);

		sNextState.write(STATE_WRITE_LOOKUP);
	} else {
		// Stay / become ready

		sNextState.write(STATE_READY);
	}
}

//-------------------------------------------------------------------------------------------------
// LFSR update process
//-------------------------------------------------------------------------------------------------
// Description:
//
// Updates the LFSR used by the random replacement strategy.
//-------------------------------------------------------------------------------------------------
// Input assertions:
//
// - rLFSR contains a valid (i.e. non-zero) state
//-------------------------------------------------------------------------------------------------
// Output assertions:
//
// - sLFSR contains new state
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::subProcessEnableLFSR() {
	// 16-bit Fibonacci LFSR

	uint oldValue = rLFSR.read();
	uint newBit = ((oldValue >> 5) & 0x1) ^ ((oldValue >> 3) & 0x1) ^ ((oldValue >> 2) & 0x1) ^ (oldValue & 0x1);
	uint newValue = (oldValue >> 1) | (newBit << 15);
	sLFSR.write(newValue);
}

//-------------------------------------------------------------------------------------------------
// Cache memory logic sensitive to negative clock edge
//-------------------------------------------------------------------------------------------------
// Description:
//
// Executes cache memory accesses based on the sCache* control and data signals. The results are
// being made available in the rCache* registers at the next negative clock edge.
//-------------------------------------------------------------------------------------------------

void SharedL1CacheBank::processCacheMemory() {
	if (sCacheLookupEnable.read()) {
		// Indicates that the cache memory shall perform a lookup operation

		bool updateLRUCounters = false;
		uint32_t updateLRUIndex;

		if (cSequentialSearch) {
			// Compare single cache tag

			uint32_t physicalSlotAddress = sCacheSetAddress.read() * cAssociativity + sCacheSetIndex.read();

			uint32_t physicalWordAddress = sCacheSetAddress.read() * cAssociativity * cCacheLineSize / 4;
			physicalWordAddress += sCacheSetIndex.read() * cCacheLineSize / 4;
			physicalWordAddress += sCacheLineAddress.read() / 4;

			if (rCellsValidFlags[physicalSlotAddress].read() && rCellsCacheTags[physicalSlotAddress].read() == sCacheTag.read()) {
				rCacheHit.write(true);
				rCacheData.write(rCellsCacheData[physicalWordAddress].read());

				updateLRUCounters = true;
				updateLRUIndex = sCacheSetIndex.read();
			} else {
				rCacheHit.write(false);
			}
		} else {
			// Compare all cache tags

			rCacheHit.write(false);

			for (uint setIndex = 0; setIndex < cAssociativity; setIndex++) {
				uint32_t physicalSlotAddress = sCacheSetAddress.read() * cAssociativity + setIndex;

				uint32_t physicalWordAddress = sCacheSetAddress.read() * cAssociativity * cCacheLineSize / 4;
				physicalWordAddress += setIndex * cCacheLineSize / 4;
				physicalWordAddress += sCacheLineAddress.read() / 4;

				if (rCellsValidFlags[physicalSlotAddress].read() && rCellsCacheTags[physicalSlotAddress].read() == sCacheTag.read()) {
					rCacheHit.write(true);
					rCacheData.write(rCellsCacheData[physicalWordAddress].read());

					updateLRUCounters = true;
					updateLRUIndex = setIndex;

					break;
				}
			}
		}

		// Update LRU counters

		if (!cRandomReplacement && updateLRUCounters) {
			uint32_t baseAddress = sCacheSetAddress.read() * cAssociativity;
			uint8_t pivotValue = rCellsLRUCounters[baseAddress + updateLRUIndex].read();

			for (uint32_t i = 0; i < cAssociativity; i++) {
				if (i == updateLRUIndex) {
					rCellsLRUCounters[baseAddress + i].write(0);
				} else if (rCellsLRUCounters[baseAddress + i].read() < pivotValue) {
					rCellsLRUCounters[baseAddress + i].write(rCellsLRUCounters[baseAddress + i].read() + 1);
				}
			}
		}
	} else if (sCacheUpdateEnable.read()) {
		// Indicates that the cache memory shall perform an update operation

		bool updateLRUCounters = false;
		uint32_t updateLRUIndex;

		if (cSequentialSearch) {
			// Compare single cache tag

			uint32_t physicalSlotAddress = sCacheSetAddress.read() * cAssociativity + sCacheSetIndex.read();

			uint32_t physicalWordAddress = sCacheSetAddress.read() * cAssociativity * cCacheLineSize / 4;
			physicalWordAddress += sCacheSetIndex.read() * cCacheLineSize / 4;
			physicalWordAddress += sCacheLineAddress.read();

			if (rCellsValidFlags[physicalSlotAddress].read() && rCellsCacheTags[physicalSlotAddress].read() == sCacheTag.read()) {
				rCacheHit.write(true);

				updateLRUCounters = true;
				updateLRUIndex = sCacheSetIndex.read();

				if (sCacheByteMask.read() == 0xF) {
					// Workaround for 64-bit instructions

					rCellsCacheData[physicalWordAddress].write(sCacheData.read());
				} else {
					// Convert byte mask to bit mask

					uint64_t writeBitMask = 0;

					switch (sCacheByteMask.read()) {
					case 0x1:	writeBitMask = 0xFF;		break;
					case 0x2:	writeBitMask = 0xFF00;		break;
					case 0x4:	writeBitMask = 0xFF0000;	break;
					case 0x8:	writeBitMask = 0xFF000000;	break;
					default:
						cerr << "Error: Invalid byte mask encountered on cache access" << endl;
						break;
					}

					uint64_t newWord = (rCellsCacheData[physicalWordAddress].read() & ~writeBitMask) | (sCacheData.read() & writeBitMask);
					rCellsCacheData[physicalWordAddress].write(newWord);
				}
			} else {
				rCacheHit.write(false);
			}
		} else {
			// Compare all cache tags

			rCacheHit.write(false);

			for (uint setIndex = 0; setIndex < cAssociativity; setIndex++) {
				uint32_t physicalSlotAddress = sCacheSetAddress.read() * cAssociativity + setIndex;

				uint32_t physicalWordAddress = sCacheSetAddress.read() * cAssociativity * cCacheLineSize / 4;
				physicalWordAddress += setIndex * cCacheLineSize / 4;
				physicalWordAddress += sCacheLineAddress.read();

				if (rCellsValidFlags[physicalSlotAddress].read() && rCellsCacheTags[physicalSlotAddress].read() == sCacheTag.read()) {
					rCacheHit.write(true);

					updateLRUCounters = true;
					updateLRUIndex = setIndex;

					if (sCacheByteMask.read() == 0xF) {
						// Workaround for 64-bit instructions

						rCellsCacheData[physicalWordAddress].write(sCacheData.read());
					} else {
						// Convert byte mask to bit mask

						uint64_t writeBitMask = 0;

						switch (sCacheByteMask.read()) {
						case 0x1:	writeBitMask = 0xFF;		break;
						case 0x2:	writeBitMask = 0xFF00;		break;
						case 0x4:	writeBitMask = 0xFF0000;	break;
						case 0x8:	writeBitMask = 0xFF000000;	break;
						default:
							cerr << "Error: Invalid byte mask encountered on cache access" << endl;
							break;
						}

						uint64_t newWord = (rCellsCacheData[physicalWordAddress].read() & ~writeBitMask) | (sCacheData.read() & writeBitMask);
						rCellsCacheData[physicalWordAddress].write(newWord);
					}

					break;
				}
			}
		}

		// Update LRU counters

		if (!cRandomReplacement && updateLRUCounters) {
			uint32_t baseAddress = sCacheSetAddress.read() * cAssociativity;
			uint8_t pivotValue = rCellsLRUCounters[baseAddress + updateLRUIndex].read();

			for (uint32_t i = 0; i < cAssociativity; i++) {
				if (i == updateLRUIndex) {
					rCellsLRUCounters[baseAddress + i].write(0);
				} else if (rCellsLRUCounters[baseAddress + i].read() < pivotValue) {
					rCellsLRUCounters[baseAddress + i].write(rCellsLRUCounters[baseAddress + i].read() + 1);
				}
			}
		}
	} else if (sCacheQueryEnable.read()) {
		// Perform a replacement query retrieving cache set index, cache tag and dirty flag

		bool selectionDone = false;

		for (uint setIndex = 0; setIndex < cAssociativity; setIndex++) {
			uint32_t physicalSlotAddress = sCacheSetAddress.read() * cAssociativity + setIndex;

			if (!rCellsValidFlags[physicalSlotAddress].read()) {
				// Invalid slot found - overrides other ways of set index selection

				rCacheSetIndex.write(setIndex);
				rCacheDirty.write(false);

				selectionDone = true;
				break;
			}
		}

		// No invalid slot found - proceed with regular selection strategy

		if (!selectionDone) {
			uint32_t physicalSlotAddress = 0;

			if (cRandomReplacement) {
				// Use externally provided set index

				physicalSlotAddress = sCacheSetAddress.read() * cAssociativity + sCacheSetIndex.read();

				rCacheSetIndex.write(sCacheSetIndex.read());

				selectionDone = true;
			} else {
				// Determine set index based on LRU counters

				for (uint setIndex = 0; setIndex < cAssociativity; setIndex++) {
					physicalSlotAddress = sCacheSetAddress.read() * cAssociativity + setIndex;

					if (rCellsLRUCounters[physicalSlotAddress].read() == cAssociativity - 1) {
						// Oldest slot found

						rCacheSetIndex.write(setIndex);

						selectionDone = true;
						break;
					}
				}
			}

			if (!selectionDone)
				cerr << "Error: Unable to select cache slot to replace" << endl;

			if (rCellsDirtyFlags[physicalSlotAddress].read()) {
				rCacheDirty.write(true);
				rCacheTag.write(rCellsCacheTags[physicalSlotAddress].read());
			} else {
				rCacheDirty.write(false);
			}
		}
	} else if (sCacheReadEnable.read()) {
		// Perform unconditional read

		uint32_t physicalWordAddress = sCacheSetAddress.read() * cAssociativity * cCacheLineSize / 4;
		physicalWordAddress += sCacheSetIndex.read() * cCacheLineSize / 4;
		physicalWordAddress += sCacheLineAddress.read();

		rCacheData.write(rCellsCacheData[physicalWordAddress].read());
	} else {
		if (sCacheStoreEnable.read()) {
			// Perform metadata update for a cache line being replaced (can be asserted in parallel to sCacheWriteEnable)

			uint32_t physicalSlotAddress = sCacheSetAddress.read() * cAssociativity + sCacheSetIndex.read();

			rCellsValidFlags[physicalSlotAddress].write(true);
			rCellsDirtyFlags[physicalSlotAddress].write(false);
			rCellsCacheTags[physicalSlotAddress].write(sCacheTag.read());

			// Update LRU counters

			if (!cRandomReplacement) {
				uint32_t baseAddress = sCacheSetAddress.read() * cAssociativity;
				uint8_t pivotValue = rCellsLRUCounters[physicalSlotAddress].read();

				for (uint32_t i = 0; i < cAssociativity; i++) {
					if (i == sCacheSetIndex.read()) {
						rCellsLRUCounters[baseAddress + i].write(0);
					} else if (rCellsLRUCounters[baseAddress + i].read() < pivotValue) {
						rCellsLRUCounters[baseAddress + i].write(rCellsLRUCounters[baseAddress + i].read() + 1);
					}
				}
			}
		}

		if (sCacheWriteEnable.read()) {
			// Perform unconditional write

			uint32_t physicalWordAddress = sCacheSetAddress.read() * cAssociativity * cCacheLineSize / 4;
			physicalWordAddress += sCacheSetIndex.read() * cCacheLineSize / 4;
			physicalWordAddress += sCacheLineAddress.read();

			rCellsCacheData[physicalWordAddress].write(sCacheData.read());
		}
	}
}

//-------------------------------------------------------------------------------------------------
// Constructors / Destructors
//-------------------------------------------------------------------------------------------------

SharedL1CacheBank::SharedL1CacheBank(sc_module_name name, ComponentID id, uint bankNumber, uint memoryBanks, uint cacheSetCount, uint associativity, uint cacheLineSize, bool sequentialSearch, bool randomReplacement) :
	Component(name, id)
{
	// Initialise configuration

	cBankNumber = bankNumber;

	uint temp = memoryBanks >> 1;
	uint bitCount = 0;
	while (temp != 0) {
		temp >>= 1;
		bitCount++;
	}

	cMemoryBanks = memoryBanks;
	cBankSelectionBits = bitCount;

	if (memoryBanks != 1UL << cBankSelectionBits) {
		cerr << "Shared L1 cache crossbar switch instantiated with memory bank count not being a power of two" << endl;
		throw std::exception();
	}

	temp = cacheLineSize >> 1;
	bitCount = 0;
	while (temp != 0) {
		temp >>= 1;
		bitCount++;
	}

	cCacheLineSize = cacheLineSize;
	cCacheLineBits = bitCount;

	if (cacheLineSize != 1UL << cCacheLineBits) {
		cerr << "Shared L1 cache bank instantiated with cache line size not being a power of two" << endl;
		throw std::exception();
	}

	if (cacheLineSize < 4) {
		cerr << "Shared L1 cache bank instantiated with cache line size being less than a single word" << endl;
		throw std::exception();
	}

	temp = cacheSetCount >> 1;
	bitCount = 0;
	while (temp != 0) {
		temp >>= 1;
		bitCount++;
	}

	cSetCount = cacheSetCount;
	cSetSelectionBits = bitCount;

	if (cacheSetCount != 1UL << cSetSelectionBits) {
		cerr << "Shared L1 cache bank instantiated with cache set count not being a power of two" << endl;
		throw std::exception();
	}

	cAssociativity = associativity;

	if (cAssociativity < 1 || cAssociativity > 16) {
		cerr << "Shared L1 cache bank instantiated with invalid associativity level" << endl;
		throw std::exception();
	}

	if (cBankSelectionBits + cCacheLineBits + cSetSelectionBits > 31) {
		cerr << "Shared L1 cache bank instantiated with overprovisioned resources" << endl;
		throw std::exception();
	}

	cMemoryBankSize = cCacheLineSize * cSetCount * cAssociativity;
	cAddressTagBits = 32 - cBankSelectionBits - cCacheLineBits - cSetSelectionBits;

	cSequentialSearch = sequentialSearch;
	cRandomReplacement = randomReplacement;

	// Initialise ports

	oAcknowledge.initialize(false);
	oReadEnable.initialize(false);
	oWriteEnable.initialize(false);

	// Initialise FSM signals

	rCurrentState.write(STATE_READY);

	if (cRandomReplacement)
		rLFSR.write(0xFFFF);

	// Initialise cache memory

	rCellsValidFlags = new sc_signal<bool>[cSetCount * cAssociativity];
	rCellsDirtyFlags = new sc_signal<bool>[cSetCount * cAssociativity];
	rCellsLRUCounters = new sc_signal<uint8_t>[cSetCount * cAssociativity];
	rCellsCacheTags = new sc_signal<uint32_t>[cSetCount * cAssociativity];
	rCellsCacheData = new sc_signal<uint64_t>[cSetCount * cAssociativity * cCacheLineSize / 4];

	for (uint i = 0; i < cSetCount * cAssociativity; i++)
		rCellsValidFlags[i].write(false);

	// Register processes

	SC_METHOD(processUtilitySignals);
	sensitive << iAddress;
	dont_initialize();

	SC_METHOD(processFSMRegisters);
	sensitive << iClock.neg();
	dont_initialize();

	SC_METHOD(processFSMCombinational);
	sensitive << iAddress << iWriteData << iByteMask << iReadEnable << iWriteEnable;
	sensitive << iReadData << iAcknowledge;
	sensitive << sInputAddressTag << sInputSetAddress << sInputBankAddress << sInputLineAddress;
	sensitive << rCurrentState << rTagCursor << rSetIndex << rLineCursor << rMemoryAddress << rLFSR;
	sensitive << rCacheHit << rCacheData << rCacheSetIndex << rCacheTag << rCacheDirty;
	dont_initialize();

	SC_METHOD(processCacheMemory);
	sensitive << iClock.neg();
	dont_initialize();

	// Indicate non-default component constructor

	end_module();
}

SharedL1CacheBank::~SharedL1CacheBank() {
	delete[] rCellsValidFlags;
	delete[] rCellsDirtyFlags;
	delete[] rCellsLRUCounters;
	delete[] rCellsCacheTags;
	delete[] rCellsCacheData;
}

//-------------------------------------------------------------------------------------------------
// Simulation utility methods - not part of simulated logic
//-------------------------------------------------------------------------------------------------

// Update the memory contents

bool SharedL1CacheBank::setWord(uint32_t address, const uint64_t data) {
	uint32_t addressTag = address & ~((1UL << cCacheLineBits) - 1);
	uint32_t setAddress = (address >> (cBankSelectionBits + cCacheLineBits)) & ((1UL << cSetSelectionBits) - 1);
	uint32_t bankAddress = (address >> cCacheLineBits) & ((1UL << cBankSelectionBits) - 1);
	uint32_t lineAddress = (address & ((1UL << cCacheLineBits) - 1)) >> 2;

	if (bankAddress != cBankNumber)
		return false;

	uint32_t physicalSlotAddress = setAddress * cAssociativity * cCacheLineSize / 4;

	for (uint setIndex = 0; setIndex < cAssociativity; setIndex++) {
		if (rCellsValidFlags[physicalSlotAddress].read() && rCellsCacheTags[physicalSlotAddress].read() == addressTag) {
			uint32_t physicalWordAddress = physicalSlotAddress + setIndex * cCacheLineSize / 4 + lineAddress;

			rCellsDirtyFlags[physicalSlotAddress].write(true);
			rCellsCacheData[physicalWordAddress].write(data);

			return true;
		}
	}

	return false;
}

// Retrieve the memory contents

bool SharedL1CacheBank::getWord(uint32_t address, uint64_t &data) {
	uint32_t addressTag = address & ~((1UL << cCacheLineBits) - 1);
	uint32_t setAddress = (address >> (cBankSelectionBits + cCacheLineBits)) & ((1UL << cSetSelectionBits) - 1);
	uint32_t bankAddress = (address >> cCacheLineBits) & ((1UL << cBankSelectionBits) - 1);
	uint32_t lineAddress = (address & ((1UL << cCacheLineBits) - 1)) >> 2;

	if (bankAddress != cBankNumber)
		return false;

	uint32_t physicalSlotAddress = setAddress * cAssociativity * cCacheLineSize / 4;

	for (uint setIndex = 0; setIndex < cAssociativity; setIndex++) {
		if (rCellsValidFlags[physicalSlotAddress].read() && rCellsCacheTags[physicalSlotAddress].read() == addressTag) {
			uint32_t physicalWordAddress = physicalSlotAddress + setIndex * cCacheLineSize / 4 + lineAddress;

			data = rCellsCacheData[physicalWordAddress].read();

			return true;
		}
	}

	return false;
}

//-------------------------------------------------------------------------------------------------
// Simulation utility methods inherited from Component - not part of simulated logic
//-------------------------------------------------------------------------------------------------

// The area of this component in square micrometres

double SharedL1CacheBank::area() const {
	cerr << "Shared L1 cache area estimation not yet implemented" << endl;
	return 0.0;
}

// The energy consumed by this component in picojoules

double SharedL1CacheBank::energy() const {
	cerr << "Shared L1 cache energy estimation not yet implemented" << endl;
	return 0.0;
}
