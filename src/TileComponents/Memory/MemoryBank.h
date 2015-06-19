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
#include "../../Datatype/MemoryRequest.h"
#include "../../Network/NetworkBuffer.h"
#include "GeneralPurposeCacheHandler.h"
#include "ReservationHandler.h"
#include "ScratchpadModeHandler.h"
#include "SimplifiedOnChipScratchpad.h"
#include "MemoryTypedefs.h"

class MemoryBank: public Component, public Blocking {
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

private:

	const uint  cMemoryBanks;             // Number of memory banks
	const uint  cBankNumber;              // Number of this memory bank (off by one)
	const bool  cRandomReplacement;       // Replace random cache lines (instead of using LRU scheme)

	//---------------------------------------------------------------------------------------------
	// Interface definitions
	//---------------------------------------------------------------------------------------------

public:

	// All data required to perform any data-access operation.
	struct RequestData_ {
	  uint32_t      Address;                    // Memory address to access
	  uint          Count;                      // Number of consecutive words to access
	  uint          TableIndex;                 // Channel map table entry used
	  ChannelIndex  ReturnChannel;              // Network address to send data to
	};
	typedef struct RequestData_ RequestData;

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
		STATE_FETCH_BURST_LENGTH,							// Fetch burst length from separate word
		STATE_LOCAL_MEMORY_ACCESS,						// Access local memory (simulate single cycle access latency)
		STATE_LOCAL_IPK_READ,								  // Read instruction packet from local memory (simulate single cycle access latency)
		STATE_GP_CACHE_MISS,								  // Access to general purpose cache caused cache miss - wait for cache FSM
	};

	enum GeneralPurposeCacheFSMState {
		GP_CACHE_STATE_PREPARE,
		GP_CACHE_STATE_SEND_DATA,
		GP_CACHE_STATE_SEND_READ_COMMAND,
		GP_CACHE_STATE_READ_DATA,
		GP_CACHE_STATE_REPLACE
	};

	struct ChannelMapTableEntry {
	public:
		bool                Valid;						// Flag indicating whether a connection is set up
		bool                FetchPending;			// Flag indicating whether a Channel ID fetch for this entry is pending
		ChannelID           ReturnChannel;		// Number of destination tile
	};

	//---------------------------------------------------------------------------------------------
	// Local state
	//---------------------------------------------------------------------------------------------

private:

	bool                  currentlyIdle;

	//-- Data queue state -------------------------------------------------------------------------

  NetworkBuffer<NetworkRequest>  mInputQueue;       // Input queue
  NetworkBuffer<NetworkResponse> mOutputQueue;      // Output queue

	bool                  mOutputWordPending;					// Indicates that an output word is waiting for acknowledgement
	NetworkData           mActiveOutputWord;					// Currently active output word

  NetworkBuffer<NetworkRequest>  mOutputReqQueue;   // Output request queue

	//-- Mode independent state -------------------------------------------------------------------

	MemoryConfig          mConfig;          // Data including associativity, line size, etc.

	FSMState              mFSMState;			  // Current FSM state
	FSMState              mFSMCallbackState;// Next FSM state to enter after sub operation is complete

	NetworkRequest        mActiveRequest;		// Currently active memory request
	RequestData           mActiveData;      // Data used to fulfil the request

	uint32_t              mRMWData;         // Temporary data for read-modify-write operations.
	bool                  mRMWDataValid;    // Does mRMWData hold valid data?
	ReservationHandler    mReservations;    // Data keeping track of current atomic transactions.

	//-- Scratchpad mode state --------------------------------------------------------------------

	ScratchpadModeHandler mScratchpadModeHandler;			// Scratchpad mode handler

	//-- Cache mode state -------------------------------------------------------------------------

	GeneralPurposeCacheHandler mGeneralPurposeCacheHandler;	// General purpose cache mode handler

	bool                  mCacheResumeRequest;	// Indicates that a memory request is being resumed after a cache update

	GeneralPurposeCacheFSMState mCacheFSMState;	// Current cache FSM state
	uint32_t              mCacheLineBuffer[256];// Cache line buffer for background memory I/O
	uint                  mCacheLineCursor;	// Index of next word in cache line buffer to process
	uint32_t              mWriteBackAddress;// Current address of write back operation in progress
	uint                  mWriteBackCount;  // Number of words to write back to background memory
	uint32_t              mFetchAddress;		// Current address of fetch operation in progress
	uint                  mFetchCount;	    // Number of words to fetch from background memory

	//-- L2 cache mode state ----------------------------------------------------------------------

	enum RequestState {
	  REQ_READY,        // Waiting for a new request
	  REQ_WAITING_FOR_BANKS, // Waiting to see if any other bank holds the data
	  REQ_WAITING_FOR_DATA,    // Waiting for data to return from main memory
	  REQ_FETCH_LINE,   // Reading a cache line and sending to another memory
	  REQ_STORE_LINE    // Storing a cache line from another memory
	};
	RequestState mRequestState;

	//---------------------------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------------------------

private:

	// Returns either the ScratchpadModeHandler or GeneralPurposeCacheHandler,
	// depending on the current configuration.
	AbstractMemoryHandler& currentMemoryHandler();

  ComponentID requestingComponent(const NetworkRequest& request) const; // The ID of the core making the current request.
  ChannelID returnChannel(const NetworkRequest& request) const; // The channel to send any response back to.
  bool isL1Request(const NetworkRequest& request) const; // Whether we are currently acting as an L1 cache.

  bool endOfCacheLine(MemoryAddr address) const;

	bool processMessageHeader();

	void processLocalMemoryAccess();
	void processLocalIPKRead();
	void prepareIPKReadOutput(AbstractMemoryHandler& handler, uint32_t data);
	void processGeneralPurposeCacheMiss();
	void processWaitRingOutput();

	void processValidInput();

	NetworkResponse assembleResponse(uint32_t data, bool endOfPacket);
	void handleDataOutput();
  void handleRequestOutput();

  void requestLoop();

  void handleNewRequest();
  void handleRequestWaitForBanks();
  void handleRequestWaitForData();
  void beginServingRequest(NetworkRequest request);
  void handleRequestFetch();
  void handleRequestStore();
  void endRequest();

	void mainLoop();										// Main loop thread - running at every positive clock edge

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

public:

	void setLocalNetwork(local_net_t* network);
	void setBackgroundMemory(SimplifiedOnChipScratchpad* memory);

	MemoryBank& bankContainingAddress(MemoryAddr addr);
	void storeData(vector<Word>& data, MemoryAddr location);
	void synchronizeData();

	void print(MemoryAddr start, MemoryAddr end);

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
