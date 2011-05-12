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
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Memory/BufferStorage.h"
#include "GeneralPurposeCacheHandler.h"
#include "ScratchpadModeHandler.h"
#include "SimplifiedOnChipScratchpad.h"

class MemoryBank: public Component {
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

private:

	uint						cChannelMapTableEntries;	// Number of available entries in the channel map table

	uint						cMemoryBanks;				// Number of memory banks

	uint						cBankNumber;				// Number of this memory bank (off by one)

	uint						cSetCount;					// Number of sets in general purpose cache mode
	uint						cWayCount;					// Number of ways in general purpose cache mode
	uint						cLineSize;					// Size of lines (for cache management and data interleaving)

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
				BankMode NewMode;
				uint GroupBaseBank;
				uint GroupIndex;
				uint GroupSize;
			} SetMode;
			struct SetTableEntry_ {
			public:
				uint TableIndex;
				ChannelID TableEntry;
			} SetTableEntry;
			struct BurstReadHandOff_ {
			public:
				uint32_t Address;
				uint Count;
				uint TableIndex;
			} BurstReadHandOff;
			struct BurstWriteForward_ {
			public:
				uint32_t Address;
				uint Count;
			} BurstWriteForward;
			struct IPKReadHandOff_ {
			public:
				uint32_t Address;
				uint TableIndex;
			} IPKReadHandOff;
			struct PassThrough_ {
			public:
				uint DestinationBankNumber;
				RingNetworkRequestType EnvelopedRequestType;
				uint32_t Address;
				uint Count;
				uint TableIndex;
			} PassThrough;
		} Header;
		struct Payload_ {
		public:
			uint32_t Data;
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

public:

	sc_in<bool>					iClock;						// Clock

	sc_out<bool>				oIdle;						// Signal that this component is not currently doing any work

	//-- Ports connected to on-chip networks ------------------------------------------------------

	sc_in<AddressedWord>		iDataIn;					// Input data sent to the memory bank
	sc_in<bool>					iDataInValid;				// Indicates that a new input data word is available
	sc_out<bool>				oDataInAcknowledge;			// Acknowledges a new input word

	sc_out<AddressedWord>		oDataOut;					// Output data sent to the processing elements
	sc_out<bool>				oDataOutValid;				// Indicates that new output data is available
	sc_in<bool>					iDataOutAcknowledge;		// Acknowledges an output word

	//-- Ports connected to background memory model -----------------------------------------------

	sc_out<bool>				oBMDataStrobe;				// Indicate that corresponding input data word is valid
	sc_out<Word>				oBMData;					// Data words input from cache controllers

	sc_in<bool>					iBMDataStrobe;				// Indicate that corresponding output data word is valid
	sc_in<Word>					iBMData;					// Data words output to cache controllers

	//-- Ports connected to memory bank ring network ----------------------------------------------

	sc_in<bool>					iRingStrobe;				// Indicates that a ring network request is pending
	sc_in<RingNetworkRequest>	iRingRequest;				// Incoming ring network request data
	sc_out<bool>				oRingAcknowledge;			// Indicates that the ring network request got consumed

	sc_out<bool>				oRingStrobe;				// Indicates that a ring network request is ready
	sc_out<RingNetworkRequest>	oRingRequest;				// Outgoing ring network request data
	sc_in<bool>					iRingAcknowledge;			// Indicates that the ring network request got consumed

	//---------------------------------------------------------------------------------------------
	// Utility definitions
	//---------------------------------------------------------------------------------------------

private:

	enum FSMState {
		STATE_IDLE,											// Ready to service incoming message
		STATE_FETCH_ADDRESS,								// Fetch address for memory access
		STATE_LOCAL_MEMORY_ACCESS,							// Access local memory (simulate single cycle access latency)
		STATE_LOCAL_IPK_READ,								// Read instruction packet from local memory (simulate single cycle access latency)
		STATE_LOCAL_BURST_READ,								// Read data burst from local memory (simulate single cycle access latency)
		STATE_BURST_WRITE_MASTER,							// Write data burst as master bank (simulate single cycle access latency)
		STATE_BURST_WRITE_SLAVE,							// Write data burst as slave bank (simulate single cycle access latency)
		STATE_GP_CACHE_MISS,								// Access to general purpose cache caused cache miss - wait for cache FSM
		STATE_WAIT_RING_OUTPUT								// Wait for ring network output buffer to become available
	};

	enum GeneralPurposeCacheFSMState {
		GP_CACHE_STATE_PREPARE,
		GP_CACHE_STATE_SEND_WRITE_COMMAND,
		GP_CACHE_STATE_SEND_WRITE_ADDRESS,
		GP_CACHE_STATE_SEND_DATA,
		GP_CACHE_STATE_SEND_READ_COMMAND,
		GP_CACHE_STATE_SEND_READ_ADDRESS,
		GP_CACHE_STATE_READ_DATA,
		GP_CACHE_STATE_REPLACE
	};

	struct ChannelMapTableEntry {
	public:
		bool Valid;											// Flag indicating whether a connection is set up
		ChannelID ReturnChannel;							// Number of destination tile
	};

	struct OutputWord {
	public:
		Word Data;											// Data word to send
		uint TableIndex;									// Index of channel map table entry describing the destination
		bool LastWord;										// Indicates whether this is the last word of the message
	};

	//---------------------------------------------------------------------------------------------
	// Local state
	//---------------------------------------------------------------------------------------------

private:

	//-- Data queue state -------------------------------------------------------------------------

	BufferStorage<AddressedWord> mInputQueue;				// Input queue
	BufferStorage<OutputWord> mOutputQueue;					// Output queue

	bool mOutputWordPending;								// Indicates that an output word is waiting for acknowledgement
	AddressedWord mActiveOutputWord;						// Currently active output word

	//-- Mode independent state -------------------------------------------------------------------

	uint mGroupBaseBank;									// Absolute index of first bank belonging to the virtual memory group
	uint mGroupIndex;										// Relative index of this bank within the virtual memory group
	uint mGroupSize;										// Total size of the virtual memory group

	ChannelMapTableEntry *mChannelMapTable;					// Channel map table containing return addresses

	BankMode mBankMode;										// Current mode of operation
	FSMState mFSMState;										// Current FSM state
	FSMState mFSMCallbackState;								// Next FSM state to enter after sub operation is complete

	uint mActiveTableIndex;									// Currently selected index in channel map table
	MemoryRequest mActiveRequest;							// Currently active memory request
	uint32_t mActiveAddress;								// Current address for memory access in progress

	bool mPartialInstructionPending;						// Indicates that half of the current instruction was already read
	uint32_t mPartialInstructionData;						// First half of instruction already read

	//-- Scratchpad mode state --------------------------------------------------------------------

	ScratchpadModeHandler mScratchpadModeHandler;			// Scratchpad mode handler

	//-- Cache mode state -------------------------------------------------------------------------

	GeneralPurposeCacheHandler mGeneralPurposeCacheHandler;	// General purpose cache mode handler

	bool mCacheResumeRequest;								// Indicates that a memory request is being resumed after a cache update

	GeneralPurposeCacheFSMState mCacheFSMState;				// Current cache FSM state
	uint32_t mCacheLineBuffer[256];							// Cache line buffer for background memory I/O
	uint mCacheLineCursor;									// Index of next word in cache line buffer to process
	uint32_t mWriteBackAddress;								// Current address of write back operation in progress
	uint mWriteBackCount;									// Number of words to write back to background memory
	uint32_t mFetchAddress;									// Current address of fetch operation in progress
	uint mFetchCount;										// Number of words to fetch from background memory

	//-- Ring network state -----------------------------------------------------------------------

	RingNetworkRequest mActiveRingRequestInput;				// Currently active incoming ring request

	bool mRingRequestOutputPending;							// Indicates that an outgoing ring request is waiting for acknowledgement
	RingNetworkRequest mActiveRingRequestOutput;			// Currently active outgoing ring request

	RingNetworkRequest mDelayedRingRequestOutput;			// Delayed outgoing ring request waiting for ring output buffer

	//---------------------------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------------------------

private:

	uint log2Exact(uint value);								// Calculates binary logarithm of value - asserts value to be a power of two greater than 1

	bool processRingEvent();
	bool processMessageHeader();

	void processAddressFetch();
	void processLocalMemoryAccess();
	void processLocalIPKRead();
	void processLocalBurstRead();
	void processBurstWriteMaster();
	void processBurstWriteSlave();
	void processGeneralPurposeCacheMiss();
	void processWaitRingOutput();

	void handleNetworkInterfacesPre();
	void handleNetworkInterfacesPost();

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

private:
	MemoryBank *mPrevMemoryBank;
	MemoryBank *mNextMemoryBank;
	SimplifiedOnChipScratchpad *mBackgroundMemory;

public:

	void setAdjacentMemories(MemoryBank *prevMemoryBank, MemoryBank *nextMemoryBank, SimplifiedOnChipScratchpad *backgroundMemory);

	void storeData(vector<Word>& data, MemoryAddr location);

	void print(MemoryAddr start, MemoryAddr end);
	Word readWord(MemoryAddr addr);
	Word readByte(MemoryAddr addr);
	void writeWord(MemoryAddr addr, Word data);
	void writeByte(MemoryAddr addr, Word data);

	virtual double area() const;						// The area of this component in square micrometres
	virtual double energy() const;						// The energy consumed by this component in picojoules
};

#endif /* MEMORYBANK_H_ */
