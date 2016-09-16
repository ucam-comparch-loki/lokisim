/*
 * MemoryBank.h
 *
 * Implementation #3.
 *
 * One bank of the banked L1/L2 cache system. A bank can be a member of an L1
 * cache or an L2 cache, but not both.
 *
 * Connections to cores are via the Data network.
 * Connections to the rest of the memory hierarchy are via Request and Response
 * networks.
 *
 *  Created on: 1 Jul 2015
 *      Author: db434
 */

#ifndef SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYBANK_H_
#define SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYBANK_H_

#include "../../Network/DelayBuffer.h"
#include "../../Utility/BlockingInterface.h"
#include "L2RequestFilter.h"
#include "MemoryBase.h"
#include "ReservationHandler.h"

class MemoryOperation;
class MainMemory;

class MemoryBank: public MemoryBase, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput            iClock;            // Clock

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

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(MemoryBank);
  MemoryBank(sc_module_name name, const ComponentID& ID, uint bankNumber);
  ~MemoryBank();

//============================================================================//
// Methods
//============================================================================//

public:

  // Compute the position in SRAM that the given memory address is to be found.
  virtual SRAMAddress getPosition(MemoryAddr address, MemoryAccessMode mode) const;

  // Return the position in the memory address space of the data stored at the
  // given position.
  virtual MemoryAddr getAddress(SRAMAddress position) const;

  // Return whether data from `address` can be found at `position` in the SRAM.
  virtual bool contains(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) const;

  // Ensure that a valid copy of data from `address` can be found at `position`.
  virtual void allocate(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode);

  // Ensure that there is a space to write data to `address` at `position`.
  virtual void validate(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode);

  // Invalidate the cache line which contains `position`.
  virtual void invalidate(SRAMAddress position, MemoryAccessMode mode);

  // Flush the cache line containing `position` down the memory hierarchy, if
  // necessary. The line is not invalidated, but is no longer dirty.
  virtual void flush(SRAMAddress position, MemoryAccessMode mode);

  // Return whether a payload flit is available. `level` tells whether this bank
  // is being treated as an L1 or L2 cache.
  virtual bool payloadAvailable(MemoryLevel level) const;

  // Retrieve a payload flit. `level` tells whether this bank is being treated
  // as an L1 or L2 cache.
  virtual uint32_t getPayload(MemoryLevel level);

  // Send a result to the requested destination.
  virtual void sendResponse(NetworkResponse response, MemoryLevel level);

  // Make a load-linked reservation.
  virtual void makeReservation(ComponentID requester, MemoryAddr address);

  // Return whether a load-linked reservation is still valid.
  virtual bool checkReservation(ComponentID requester, MemoryAddr address) const;

  // Check whether it is safe for the given operation to modify memory.
  virtual void preWriteCheck(const MemoryOperation& operation) const;

  // Override writeWord so we can update metadata (valid, dirty, etc.).
  virtual void writeWord(SRAMAddress position, uint32_t data);

  // Access data based on its position in the address space, and bypass the
  // usual tag checks.
  Word readWordDebug(MemoryAddr addr);
  Word readByteDebug(MemoryAddr addr);
  void writeWordDebug(MemoryAddr addr, Word data);
  void writeByteDebug(MemoryAddr addr, Word data);

  void setLocalNetwork(local_net_t* network);
  void setBackgroundMemory(MainMemory* memory);

  void storeData(vector<Word>& data, MemoryAddr location);
  void synchronizeData();

  void print(MemoryAddr start, MemoryAddr end);

  bool storedLocally(MemoryAddr address) const;

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

  void copyToMissBuffer();

  void processValidInput();
  void handleDataOutput();
  void handleRequestOutput();

  void mainLoop();                    // Main loop thread

  void updateIdle();                  // Update idleness
  void updateReady();                 // Update flow control signals

protected:

  virtual void reportStalls(ostream& os);

//============================================================================//
// Local state
//============================================================================//

private:

  enum MemoryState {
    STATE_IDLE,                           // No active request
    STATE_REQUEST,                        // Serving active request
    STATE_ALLOCATE,                       // Allocating cache line
    STATE_FLUSH,                          // Flushing cache line
    STATE_REFILL,                         // Filling cache line
    STATE_FORWARD,                        // Forward request to directory or L2
  };

  MemoryState           mState, mPreviousState;

  bool                  currentlyIdle;

  NetworkBuffer<NetworkRequest>  mInputQueue;       // Input queue
  DelayBuffer<NetworkResponse>   mOutputQueue;      // Output queue
  DelayBuffer<NetworkRequest>    mOutputReqQueue;   // Output request queue

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

  // L2 requests are broadcast to all banks to allow high associativity. This
  // module filters out requests which are for this bank.
  L2RequestFilter       mL2RequestFilter;

  // Magic connection to main memory.
  MainMemory *mMainMemory;

  // Pointer to network, allowing new interfaces to be experimented with quickly.
  local_net_t *localNetwork;

  // Data received through the L2 request filter.
  RequestSignal requestSig;


};

#endif /* SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYBANK_H_ */
