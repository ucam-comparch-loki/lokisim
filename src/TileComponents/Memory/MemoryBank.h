//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Configurable Memory Bank Definition
//-------------------------------------------------------------------------------------------------
// Second version of configurable memory bank model. Each memory bank is
// directly connected to the network. There are multiple memory banks per tile.
//
// Memory requests are connection-less. Instead, the new channel map table
// mechanism in the memory bank is used.
//
// The number of input and output ports is fixed to 1. The ports possess
// queues for incoming and outgoing data.
//-------------------------------------------------------------------------------------------------
// File:       MemoryBank.h
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 08/04/2011
//-------------------------------------------------------------------------------------------------

#ifndef MEMORYBANK_H_
#define MEMORYBANK_H_

#include "../../Component.h"
#include "../../Utility/Blocking.h"
#include "../../Network/DelayBuffer.h"
#include "CacheLineBuffer.h"
#include "GeneralPurposeCacheHandler.h"
#include "L2RequestFilter.h"
#include "ReservationHandler.h"
#include "ScratchpadModeHandler.h"
#include "MemoryTypedefs.h"

class SimplifiedOnChipScratchpad;

class MemoryBank: public Component, public Blocking {
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

private:

	const uint  cMemoryBanks;             // Number of memory banks
	const uint  cBankNumber;              // Number of this memory bank (off by one)
	const bool  cRandomReplacement;       // Replace random cache lines (instead of using LRU scheme)


	//---------------------------------------------------------------------------------------------
	// Ports
	//---------------------------------------------------------------------------------------------

public:

	ClockInput					  iClock;						// Clock

	//-- Ports connected to on-chip networks ------------------------------------------------------

	// Data - to/from cores on the same tile.
  DataInput             iData;            // Input data sent to the memory bank
  ReadyOutput           oReadyForData;    // Indicates that there is buffer space for new input
  DataOutput            oData;            // Output data sent to the processing elements

  // Requests - to/from memory banks on other tiles.
  RequestInput          iRequest;         // Input requests sent to the memory bank
  sc_in<MemoryIndex>    iRequestTarget;   // The responsible bank if all banks miss
  RequestOutput         oRequest;         // Output requests sent to the remote memory banks

  sc_in<bool>           iRequestClaimed;  // One of the banks has claimed the request.
  sc_out<bool>          oClaimRequest;    // Tell whether this bank has claimed the request.

  // Responses - to/from memory banks on other tiles.
  ResponseInput         iResponse;
  sc_in<MemoryIndex>    iResponseTarget;
  ResponseOutput        oResponse;        // Output responses sent to the remote memory banks

	//---------------------------------------------------------------------------------------------
	// Utility definitions
	//---------------------------------------------------------------------------------------------

private:

	enum FSMState {
		STATE_IDLE,											      // Ready to service incoming message
		STATE_LOCAL_MEMORY_ACCESS,						// Access local memory (simulate single cycle access latency)
		STATE_CACHE_FLUSH                     // Start flushing a cache line next clock cycle
	};

	// Inputs/outputs to receive/send request data on.
	enum RequestSource {
	  REQUEST_CORES,
	  REQUEST_MEMORIES,
	  REQUEST_REFILL,
	  REQUEST_NONE
	};

	struct ChannelMapTableEntry {
	public:
		bool                Valid;						// Flag indicating whether a connection is set up
		bool                FetchPending;			// Flag indicating whether a Channel ID fetch for this entry is pending
		ChannelID           ReturnChannel;		// Number of destination tile
	};

  // All data required to perform any data-access operation.
  struct RequestData {
	  NetworkRequest      Request;          // The original request
    MemoryAddr          Address;          // Memory address to access
    SRAMAddress         Position;         // Position of the data in SRAM
    FSMState            State;            // Position in state machine
    RequestSource       Source;           // Where the request came from
    ChannelID           ReturnChannel;    // Address to send responses to
    uint                FlitsSent;        // Number of response flits sent so far
    bool                Complete;         // Whether the request has been served
  };

	//---------------------------------------------------------------------------------------------
	// Local state
	//---------------------------------------------------------------------------------------------

private:

	bool                  currentlyIdle;

	//-- Data queue state -------------------------------------------------------------------------

  NetworkBuffer<NetworkRequest>  mInputQueue;       // Input queue
  DelayBuffer<NetworkResponse>   mOutputQueue;      // Output queue

	bool                  mOutputWordPending;					// Indicates that an output word is waiting for acknowledgement
	NetworkData           mActiveOutputWord;					// Currently active output word

  DelayBuffer<NetworkRequest>    mOutputReqQueue;   // Output request queue

	//-- Mode independent state -------------------------------------------------------------------

  vector<uint32_t>      mData;            // The contents of this bank.

	MemoryConfig          mConfig;          // Data including associativity, line size, etc.

	RequestData           mActiveData;      // Data used to fulfil the request

	// Callback request - put on hold while performing sub-operations such as
	// cache line fetches.
	RequestData           mCallbackData;


	uint32_t              mRMWData;         // Temporary data for read-modify-write operations.
	bool                  mRMWDataValid;    // Does mRMWData hold valid data?
	ReservationHandler    mReservations;    // Data keeping track of current atomic transactions.

	//-- Scratchpad mode state --------------------------------------------------------------------

	ScratchpadModeHandler mScratchpadModeHandler;			// Scratchpad mode handler

	//-- Cache mode state -------------------------------------------------------------------------

	GeneralPurposeCacheHandler mGeneralPurposeCacheHandler;	// General purpose cache mode handler

	CacheLineBuffer       mCacheLineBuffer; // Used to make cache line operations more efficient

	//-- L2 cache mode state ----------------------------------------------------------------------

	// L2 requests are broadcast to all banks to allow high associativity. This
	// module filters out requests which are for this bank.
	L2RequestFilter       mL2RequestFilter;

	//---------------------------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------------------------

private:

	// Returns either the ScratchpadModeHandler or GeneralPurposeCacheHandler,
	// depending on the current configuration.
	AbstractMemoryHandler& currentMemoryHandler();
	bool checkTags() const;

  ChannelID returnChannel(const NetworkRequest& request) const; // The channel to send any response back to.
  bool inL1Mode() const; // Whether we are currently acting as an L1 cache.
  bool onlyAcceptingRefills() const;  // If we don't support hit-under-miss, we must wait for data to return

  MemoryAddr getTag(MemoryAddr address) const;
  MemoryAddr getOffset(MemoryAddr address) const;
  bool sameCacheLine(MemoryAddr address1, MemoryAddr address2) const;
  bool endOfCacheLine(MemoryAddr address) const;

	bool startNewRequest();
	bool processMessageHeader(const NetworkRequest& request);

	// Ensure that the cache line containing the given address is stored locally.
	//   validate = don't fetch the line - it will be received by other means
	//   required = if the cache line is not already in the cache, do nothing
	//   isRead, isInstruction = the type of memory access, used for debug
	void getCacheLine(MemoryAddr address, bool validate, bool required, bool isRead, bool isInstruction);

	// Main function, farms work out to handlers below.
	void processLocalMemoryAccess();

	// A handler for each possible operation.
	void processLoadWord();
	void processLoadHalfWord();
	void processLoadByte();
	void processStoreWord();
	void processStoreHalfWord();
	void processStoreByte();
	void processIPKRead();
	void processFetchLine();
	void processStoreLine();
	void processFlushLine();
	void processMemsetLine();
	void processInvalidateLine();
	void processValidateLine();
	void processLoadLinked();
	void processStoreConditional();
	void processLoadAndAdd();
	void processLoadAndOr();
	void processLoadAndAnd();
	void processLoadAndXor();
	void processExchange();

	// Helper functions for the operation handlers.

	void processLoadResult(uint32_t data, bool endOfPacket);
	void processStoreResult(uint32_t data);
	void processRMWPhase1();
	void processRMWPhase2(uint32_t data);

	uint32_t getDataToStore();
	bool inputAvailable() const;
	const NetworkRequest peekInput();
	const NetworkRequest consumeInput();
	bool canSend() const;
	void send(const NetworkData& data);

	// Move data from cache to cache line buffer.
	void prepareCacheLineBuffer(MemoryAddr address);

	// Read next word from cache line buffer.
	uint32_t readFromCacheLineBuffer();

	// Write next word to cache line buffer.
	void writeToCacheLineBuffer(uint32_t data);

	// Move data from cache line buffer to cache.
	void replaceCacheLine();

	void processDelayedCacheFlush();

	void processValidInput();

	NetworkResponse assembleResponse(uint32_t data, bool endOfPacket);
	void handleDataOutput();
  void handleRequestOutput();

	void mainLoop();										// Main loop thread

	void updateIdle();                  // Update idleness
	void updateReady();                 // Update flow control signals

	//---------------------------------------------------------------------------------------------
	// Constructors and destructors
	//---------------------------------------------------------------------------------------------

public:

	SC_HAS_PROCESS(MemoryBank);
	MemoryBank(sc_module_name name, const ComponentID& ID, uint bankNumber);
	~MemoryBank();

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods inherited from Component - not part of simulated logic
	//---------------------------------------------------------------------------------------------

protected:
	SimplifiedOnChipScratchpad *mBackgroundMemory;

	// Pointer to network, allowing new interfaces to be experimented with quickly.
	local_net_t *localNetwork;

	// Data received through the L2 request filter.
	RequestSignal requestSig;

public:

	void setLocalNetwork(local_net_t* network);
	void setBackgroundMemory(SimplifiedOnChipScratchpad* memory);

	void storeData(vector<Word>& data, MemoryAddr location);
	void synchronizeData();

	void print(MemoryAddr start, MemoryAddr end);

  bool storedLocally(MemoryAddr address) const;

	Word readWord(MemoryAddr addr);
	Word readByte(MemoryAddr addr);
	void writeWord(MemoryAddr addr, Word data);
	void writeByte(MemoryAddr addr, Word data);

protected:

	virtual void reportStalls(ostream& os);

private:

	void printOperation(MemoryRequest::MemoryOperation operation, MemoryAddr address, uint32_t data) const;

};

#endif /* MEMORYBANK_H_ */
