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
#include "ScratchpadModeHandler.h"
#include "SimplifiedOnChipScratchpad.h"
#include "../../Datatype/Packets/Packet.h"

class MemoryBank: public Component, public Blocking {
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

private:

	uint						cChannelMapTableEntries;	// Number of available entries in the channel map table

	uint						cMemoryBanks;				// Number of memory banks

	uint						cBankNumber;				// Number of this memory bank (off by one)

	uint						mWayCount;					// Number of ways in general purpose cache mode
	uint						mLineSize;					// Size of lines (for cache management and data interleaving)

	bool						cRandomReplacement;			// Replace random cache lines (instead of using LRU scheme)

	//---------------------------------------------------------------------------------------------
	// Interface definitions
	//---------------------------------------------------------------------------------------------

public:

	enum BankMode {
		MODE_INACTIVE,
		MODE_SCRATCHPAD,
		MODE_GP_CACHE
	};

	enum RingNetworkRequestType {
		RING_SET_MODE,
		RING_SET_TABLE_ENTRY,
		RING_BURST_READ_HAND_OFF,
		RING_BURST_WRITE_FORWARD,
		RING_IPK_READ_HAND_OFF,
		RING_PASS_THROUGH
	};

	struct RingNetworkRequest {
	public:
		struct Header_ {
		public:
			RingNetworkRequestType RequestType;
			struct SetMode_ {
			public:
				BankMode        NewMode;
				uint            WayCount;
				uint            LineSize;
				uint            GroupBaseBank;
				uint            GroupIndex;
				uint            GroupSize;
			} SetMode;
			struct SetTableEntry_ {
			public:
				uint            TableIndex;
				ChannelID       TableEntry;
			} SetTableEntry;
			struct BurstReadHandOff_ {
			public:
				uint32_t        Address;
				uint            Count;
				uint            TableIndex;
				ChannelIndex    ReturnChannel;
			} BurstReadHandOff;
			struct BurstWriteForward_ {
			public:
				uint32_t        Address;
				uint            Count;
			} BurstWriteForward;
			struct IPKReadHandOff_ {
			public:
				uint32_t        Address;
				uint            TableIndex;
				ChannelIndex    ReturnChannel;
				bool            PartialInstructionPending;
				uint32_t        PartialInstructionData;
			} IPKReadHandOff;
			struct PassThrough_ {
			public:
				uint            DestinationBankNumber;
				RingNetworkRequestType EnvelopedRequestType;
				uint32_t        Address;
				uint            Count;
				uint            TableIndex;
				ChannelIndex    ReturnChannel;
				bool            PartialInstructionPending;
				uint32_t        PartialInstructionData;
			} PassThrough;
		} Header;
		struct Payload_ {
		public:
			uint32_t          Data;
		} Payload;

		friend void sc_trace(sc_core::sc_trace_file*& tf, const RingNetworkRequest& w, const std::string& txt) {
			// Nothing
		}

		bool operator== (const RingNetworkRequest& other) const {
			return false;
		}

		//RingNetworkRequest& operator= (const RingNetworkRequest& other);

		friend std::ostream& operator<< (std::ostream& os, RingNetworkRequest const& v) {
			os << "Ring request";
			return os;
		}
	};

	//---------------------------------------------------------------------------------------------
	// Ports
	//---------------------------------------------------------------------------------------------

private:

	// For communications with the Miss Handling Logic, no additional network
	// information is required, so we send just words rather than flits.
  typedef loki_in<MemoryRequest> InRequestPort;
  typedef loki_out<MemoryRequest> OutRequestPort;
	typedef loki_in<Word> InResponsePort;
  typedef loki_out<Word> OutResponsePort;

public:

	ClockInput					  iClock;						// Clock

	//-- Ports connected to on-chip networks ------------------------------------------------------

	// Data - to/from cores on the same tile.
  DataInput             iData;            // Input data sent to the memory bank
  ReadyOutput           oReadyForData;    // Indicates that there is buffer space for new input
  DataOutput            oData;            // Output data sent to the processing elements

  // Requests - to/from memory banks on other tiles.
  InRequestPort         iRequest;         // Input requests sent to the memory bank
  ReadyOutput           oReadyForRequest; // Indicates that there is buffer space for new input
  OutRequestPort        oRequest;         // Output requests sent to the remote memory banks

  // Responses - to/from memory banks on other tiles.
  InResponsePort        iResponse;
  OutResponsePort       oResponse;        // Output responses sent to the remote memory banks

	//-- Ports connected to memory bank ring network ----------------------------------------------

	sc_in<bool>					  iRingStrobe;			// Indicates that a ring network request is pending
	sc_in<RingNetworkRequest>	iRingRequest;	// Incoming ring network request data
	sc_out<bool>				  oRingAcknowledge;	// Indicates that the ring network request got consumed

	sc_out<bool>				  oRingStrobe;			// Indicates that a ring network request is ready
	sc_out<RingNetworkRequest> oRingRequest;// Outgoing ring network request data
	sc_in<bool>					  iRingAcknowledge;	// Indicates that the ring network request got consumed

	//---------------------------------------------------------------------------------------------
	// Utility definitions
	//---------------------------------------------------------------------------------------------

private:

	enum FSMState {
		STATE_IDLE,											      // Ready to service incoming message
		STATE_FETCH_BURST_LENGTH,							// Fetch burst length from separate word
		STATE_LOCAL_MEMORY_ACCESS,						// Access local memory (simulate single cycle access latency)
		STATE_LOCAL_IPK_READ,								  // Read instruction packet from local memory (simulate single cycle access latency)
		STATE_LOCAL_BURST_READ,								// Read data burst from local memory (simulate single cycle access latency)
		STATE_BURST_WRITE_MASTER,							// Write data burst as master bank (simulate single cycle access latency)
		STATE_BURST_WRITE_SLAVE,							// Write data burst as slave bank (simulate single cycle access latency)
		STATE_GP_CACHE_MISS,								  // Access to general purpose cache caused cache miss - wait for cache FSM
		STATE_WAIT_RING_OUTPUT								// Wait for ring network output buffer to become available
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

	struct OutputWord {
	public:
		Word                Data;							// Data word to send
		uint                TableIndex;				// Index of channel map table entry describing the destination
		ChannelIndex        ReturnChannel;    // Channel to send to when rest of return address is implicit
		bool                PortClaim;				// Indicates whether this is a port claim message
		bool                LastWord;					// Indicates whether this is the last word of the message
	};

	//---------------------------------------------------------------------------------------------
	// Local state
	//---------------------------------------------------------------------------------------------

private:

	bool                  currentlyIdle;

	// Keep track of which input we are currently serving a request from.
	// Requests from memories have priority, but once a request starts, it should
	// continue to completion.
	enum PriorityInput {INPUT_NONE, INPUT_CORES, INPUT_MEMORIES};
	PriorityInput         mCurrentInput;

	//-- Data queue state -------------------------------------------------------------------------

  NetworkBuffer<NetworkData>     mInputQueue;       // Input queue
  NetworkBuffer<OutputWord>      mOutputQueue;      // Output queue

	bool                  mOutputWordPending;					// Indicates that an output word is waiting for acknowledgement
	NetworkData           mActiveOutputWord;					// Currently active output word

  NetworkBuffer<NetworkRequest>  mInputReqQueue;    // Input request queue
  NetworkBuffer<MemoryRequest>   mOutputReqQueue;   // Output request queue

  NetworkBuffer<NetworkResponse> mOutputRespQueue;  // Output response queue

  Packet<Word>         *mCurrentPacket;   // The packet we are currently sending or receiving

	//-- Mode independent state -------------------------------------------------------------------

	uint                  mGroupBaseBank;	  // Absolute index of first bank belonging to the virtual memory group
	uint                  mGroupIndex;	    // Relative index of this bank within the virtual memory group
	uint                  mGroupSize;		    // Total size of the virtual memory group

	ChannelMapTableEntry *mChannelMapTable;	// Channel map table containing return addresses

	BankMode              mBankMode;			  // Current mode of operation
	FSMState              mFSMState;			  // Current FSM state
	FSMState              mFSMCallbackState;// Next FSM state to enter after sub operation is complete

	uint                  mActiveTableIndex;// Currently selected index in channel map table
	ChannelIndex          mActiveReturnChannel; // Current specified return channel (if supplied)
	MemoryRequest         mActiveRequest;		// Currently active memory request
	uint32_t              mActiveAddress;		// Current address for memory access in progress
	uint32_t              mActiveBurstLength; 	// Current burst length for memory access in progress

	bool                  mPartialInstructionPending;	// Indicates that half of the current instruction was already read
	uint32_t              mPartialInstructionData;		// First half of instruction already read

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
	  REQ_FETCH_LINE,   // Reading a cache line and sending to another memory
	  REQ_STORE_LINE    // Storing a cache line from another memory
	};
	RequestState requestState;

	//-- Ring network state -----------------------------------------------------------------------

	bool                  mRingRequestInputPending;   // Indicates that an incoming ring request is waiting to be processed
	RingNetworkRequest    mActiveRingRequestInput;		// Currently active incoming ring request

	bool                  mRingRequestOutputPending;  // Indicates that an outgoing ring request is waiting for acknowledgement
	RingNetworkRequest    mActiveRingRequestOutput;   // Currently active outgoing ring request

	RingNetworkRequest    mDelayedRingRequestOutput;	// Delayed outgoing ring request waiting for ring output buffer

	//---------------------------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------------------------

private:

	uint log2Exact(uint value);							// Calculates binary logarithm of value - asserts value to be a power of two greater than 1

	// There are multiple buffers which could hold requests. Use these methods to
	// ensure that the correct buffer is accessed.
	const NetworkData peekNextRequest();
	const NetworkData readNextRequest();
	bool currentInputEmpty();

	bool processRingEvent();
	bool processMessageHeader();

	void processFetchBurstLength();
	void processLocalMemoryAccess();
	void processLocalIPKRead();
	void processLocalBurstRead();
	void processBurstWriteMaster();
	void processBurstWriteSlave();
	void processGeneralPurposeCacheMiss();
	void processWaitRingOutput();

	void processValidRing();
	void processValidInput();

	void handleNetworkInterfacesPre();
	void handleDataOutput();
  void handleRequestInput();
  void handleRequestOutput();
  void handleResponseInput();
  void handleResponseOutput();

  void requestLoop();

  void handleNewRequest();
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
	MemoryBank *mPrevMemoryBank;
	MemoryBank *mNextMemoryBank;
	SimplifiedOnChipScratchpad *mBackgroundMemory;

	// Pointer to network, allowing new interfaces to be experimented with quickly.
	local_net_t *localNetwork;

public:

	void setAdjacentMemories(MemoryBank *prevMemoryBank, MemoryBank *nextMemoryBank, SimplifiedOnChipScratchpad *backgroundMemory);
	void setLocalNetwork(local_net_t* network);

	void storeData(vector<Word>& data, MemoryAddr location);
	void synchronizeData();

	void print(MemoryAddr start, MemoryAddr end);
	Word readWord(MemoryAddr addr);
	Word readByte(MemoryAddr addr);
	void writeWord(MemoryAddr addr, Word data);
	void writeByte(MemoryAddr addr, Word data);

protected:

	virtual void reportStalls(ostream& os);

};

#endif /* MEMORYBANK_H_ */
