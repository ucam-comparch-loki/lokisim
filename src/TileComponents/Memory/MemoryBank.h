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
#include "L2RequestFilter.h"
#include "ReservationHandler.h"
#include "MemoryTypedefs.h"

class MemoryOperation;
class SimplifiedOnChipScratchpad;

class MemoryBank: public Component, public Blocking {
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
  // Software interface
  //---------------------------------------------------------------------------------------------

public:

  // Compute the position in SRAM that the given memory address is to be found.
  SRAMAddress getPosition(MemoryAddr address, MemoryAccessMode mode) const;

  // Return whether data from `address` can be found at `position` in the SRAM.
  bool contains(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) const;

  // Ensure that a valid copy of data from `address` can be found at `position`.
  void allocate(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode);

  // Ensure that there is a space to write data to `address` at `position`.
  void validate(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode);

  // Invalidate the cache line which contains `position`.
  void invalidate(SRAMAddress position, MemoryAccessMode mode);

  // Flush the cache line containing `position` down the memory hierarchy, if
  // necessary. The line is not invalidated, but is no longer dirty.
  void flush(SRAMAddress position, MemoryAccessMode mode);

  // Return whether a payload flit is available. `level` tells whether this bank
  // is being treated as an L1 or L2 cache.
  bool payloadAvailable(MemoryLevel level) const;

  // Retrieve a payload flit. `level` tells whether this bank is being treated
  // as an L1 or L2 cache.
  uint32_t getPayload(MemoryLevel level);

  // Send a result to the requested destination.
  void sendResponse(NetworkResponse response, MemoryLevel level);

  // Make a load-linked reservation.
  void makeReservation(ComponentID requester, MemoryAddr address);

  // Return whether a load-linked reservation is still valid.
  bool checkReservation(ComponentID requester, MemoryAddr address) const;

  // Check whether it is safe to write to the given address.
  void preWriteCheck(MemoryAddr address) const;

  uint32_t readWord(SRAMAddress position) const;
  uint32_t readHalfword(SRAMAddress position) const;
  uint32_t readByte(SRAMAddress position) const;
  void writeWord(SRAMAddress position, uint32_t data);
  void writeHalfword(SRAMAddress position, uint32_t data);
  void writeByte(SRAMAddress position, uint32_t data);

  MemoryAddr getTag(MemoryAddr address) const;
  MemoryAddr getLine(MemoryAddr address) const;
  MemoryAddr getOffset(MemoryAddr address) const;

  //---------------------------------------------------------------------------------------------
  // Utility definitions
  //---------------------------------------------------------------------------------------------

private:

  enum MemoryState {
    STATE_IDLE,                           // No active request
    STATE_REQUEST,                        // Serving active request
    STATE_ALLOCATE,                       // Allocating cache line
    STATE_FLUSH,                          // Flushing cache line
    STATE_REFILL,                         // Filling cache line
    STATE_FORWARD,                        // Forward request to directory or L2
  };

	//---------------------------------------------------------------------------------------------
	// Local state
	//---------------------------------------------------------------------------------------------

private:

	bool                  currentlyIdle;

	MemoryState           mState, mPreviousState;

	//-- Data queue state -------------------------------------------------------------------------

  NetworkBuffer<NetworkRequest>  mInputQueue;       // Input queue
  DelayBuffer<NetworkResponse>   mOutputQueue;      // Output queue

	bool                  mOutputWordPending;					// Indicates that an output word is waiting for acknowledgement
	NetworkData           mActiveOutputWord;					// Currently active output word

  DelayBuffer<NetworkRequest>    mOutputReqQueue;   // Output request queue

	//-- Mode independent state -------------------------------------------------------------------

  vector<uint32_t>      mData;            // The contents of this bank.
  vector<MemoryTag>     mTags;            // Cache tags for each line.
  vector<bool>          mValid;           // Valid data flag for each line.
  vector<bool>          mDirty;           // Modified data flag for each line.

	MemoryOperation*      mActiveRequest;   // The request being served.

	// Callback request - put on hold while performing sub-operations such as
	// cache line fetches.
	MemoryOperation*      mMissingRequest;

	ReservationHandler    mReservations;    // Data keeping track of current atomic transactions.

	unsigned int          mCacheLineCursor; // Used to step through cache lines.

	BufferStorage<NetworkRequest> mMissBuffer; // Payloads for a request which is currently missing.

	bool                  mCopyToMissBuffer;   // Tell whether the miss buffer needs filling.
	bool                  mReadFromMissBuffer; // Tell whether the miss buffer needs emptying.

	sc_event              mCacheMissEvent;  // Event triggered on each cache miss.

	//-- L2 cache mode state ----------------------------------------------------------------------

	// L2 requests are broadcast to all banks to allow high associativity. This
	// module filters out requests which are for this bank.
	L2RequestFilter       mL2RequestFilter;

	//---------------------------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------------------------

private:

	// Tasks to carry out for each of the possible states.
	void processIdle();
	void processRequest();
	void processAllocate();
	void processFlush();
	void processRefill();
	void processForward();

	// Perform any tidying necessary when a request finishes.
	void finishedRequest();

	bool requestAvailable() const;
	const sc_event_or_list& requestAvailableEvent() const;
	MemoryOperation* peekRequest();
	void consumeRequest(MemoryLevel level);

	bool canSendRequest() const;
	const sc_event& canSendRequestEvent() const;
	void sendRequest(NetworkRequest request);

  bool responseAvailable() const;
  const sc_event& responseAvailableEvent() const;
  uint32_t getResponse();

  bool canSendResponse(MemoryLevel level) const;
  const sc_event& canSendResponseEvent(MemoryLevel level) const;
  // sendResponse is part of the public interface, above

  bool isPayload(NetworkRequest request) const;

  void copyToMissBuffer();

	void processValidInput();
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

	Word readWordDebug(MemoryAddr addr);
	Word readByteDebug(MemoryAddr addr);
	void writeWordDebug(MemoryAddr addr, Word data);
	void writeByteDebug(MemoryAddr addr, Word data);

protected:

	virtual void reportStalls(ostream& os);

public:

	void printOperation(MemoryOpcode operation, MemoryAddr address, uint32_t data) const;

};

#endif /* MEMORYBANK_H_ */
