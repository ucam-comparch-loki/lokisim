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

#include "../../Memory/MemoryBase.h"
#include "L2RequestFilter.h"
#include "ReservationHandler.h"
#include "../../Network/FIFOs/DelayFIFO.h"
#include "../../Network/FIFOs/NetworkFIFO.h"
#include "../../Utility/BlockingInterface.h"
#include "../../Utility/LokiVector.h"
#include "../Network/L2LToBankRequests.h"

class ComputeTile;
class MemoryOperation;
class MainMemory;

class MemoryBank: public MemoryBase, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput            iClock;           // Clock

  // Data - to/from cores on the same tile.
  sc_port<network_sink_ifc<Word>> iData;  // Input data sent to the memory bank
  sc_port<network_source_ifc<Word>> oData;        // Data sent to the cores
  sc_port<network_source_ifc<Word>> oInstruction; // Instructions sent to the cores

  // Requests - to/from memory banks on other tiles.
  sc_port<network_sink_ifc<Word>> iRequest;   // Input requests sent to the memory bank
  sc_port<network_source_ifc<Word>> oRequest; // Output requests sent to the remote memory banks

  sc_port<l2_request_bank_ifc> l2Associativity; // Interface for coordinating L2 requests

  // Responses - to/from memory banks on other tiles.
  sc_port<network_sink_ifc<Word>> iResponse;
  sc_port<network_source_ifc<Word>> oResponse; // Output responses sent to the remote memory banks

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(MemoryBank);
  MemoryBank(sc_module_name name, const ComponentID& ID, uint numBanks,
             const memory_bank_parameters_t& params, uint numCores);
  ~MemoryBank();

//============================================================================//
// Methods
//============================================================================//

public:

  // The number of cache lines stored in this MemoryBank.
  size_t numCacheLines() const;
  size_t cacheLineSize() const;

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
  virtual void makeReservation(ComponentID requester, MemoryAddr address, MemoryAccessMode mode);

  // Return whether a load-linked reservation is still valid.
  virtual bool checkReservation(ComponentID requester, MemoryAddr address, MemoryAccessMode mode) const;

  // Check whether it is safe for the given operation to modify memory.
  virtual void preWriteCheck(const MemoryOperation& operation) const;

  // Override writeWord so we can update metadata (valid, dirty, etc.).
  virtual void writeWord(SRAMAddress position, uint32_t data, MemoryAccessMode mode);

  // Event triggered every time a request flit is sent.
  const sc_event& requestSentEvent() const;

  // Determine whether we are currently flushing data from the given address.
  bool flushing(MemoryAddr address) const;

  // Access data based on its position in the address space, and bypass the
  // usual tag checks.
  Word readWordDebug(MemoryAddr addr);
  Word readByteDebug(MemoryAddr addr);
  void writeWordDebug(MemoryAddr addr, Word data);
  void writeByteDebug(MemoryAddr addr, Word data);

  void setBackgroundMemory(MainMemory* memory);

  void synchronizeData();

  void print(MemoryAddr start, MemoryAddr end);

  bool storedLocally(MemoryAddr address) const;

  // The index of this memory bank, with the first bank being 0.
  uint memoryIndex() const;
  uint memoriesThisTile() const;
  uint globalMemoryIndex() const;

  // For debug/statistics.
  bool isCore(ChannelID destination) const;
  uint coresThisTile() const;
  uint globalCoreIndex(ComponentID core) const;

private:

  typedef std::shared_ptr<MemoryOperation> DecodedRequest;

  // Tasks to carry out for each of the possible states.
  void processIdle();
  void processRequest(DecodedRequest& request);
  void processAllocate(DecodedRequest& request);
  void processFlush(DecodedRequest& request);
  void processRefill(DecodedRequest& request);
  void processForward(DecodedRequest& request);

  // Perform any tidying necessary when a request finishes.
  void finishedRequest(DecodedRequest& request);
  // We have done all the work on a request that we can do at the moment, but
  // will come back to it later.
  void finishedRequestForNow(DecodedRequest& request);

  bool requestAvailable() const;
  const sc_event_or_list& requestAvailableEvent() const;
  DecodedRequest peekRequest();
  void consumeRequest(MemoryLevel level);

  bool canSendRequest() const;
  const sc_event& canSendRequestEvent() const;
  void sendRequest(NetworkRequest request);
  void forwardRequest(NetworkRequest request);

  bool responseAvailable() const;
  const sc_event& responseAvailableEvent() const;
  uint32_t getResponse();

  bool canSendResponse(ChannelID destination, MemoryLevel level) const;
  const sc_event& canSendResponseEvent(ChannelID destination, MemoryLevel level) const;
  // sendResponse is part of the public interface, above

  void copyToMissBuffer();

  void coreRequestArrived();
  void coreDataSent();
  void coreInstructionSent();
  void memoryRequestSent();

  void mainLoop();                    // Main loop thread

  void updateIdle();                  // Update idleness

  // Determine how long outputs must be delayed to achieve the required latency.
  static cycle_count_t artificialDelayRequired(const memory_bank_parameters_t& params);

  // Determine how large the request queue must be to prevent deadlock.
  static size_t requestQueueSize(const memory_bank_parameters_t& params);

  ComputeTile& parent() const;
  Chip& chip() const;

protected:

  virtual const vector<uint32_t>& dataArray() const;
  virtual vector<uint32_t>& dataArray();

  virtual void reportStalls(ostream& os);

//============================================================================//
// Local state
//============================================================================//

private:

  // Configuration
  const bool hitUnderMiss;
  const size_t log2NumBanks;

  enum MemoryState {
    STATE_IDLE,                          // No active request
    STATE_REQUEST,                       // Serving active request
    STATE_ALLOCATE,                      // Allocating cache line
    STATE_FLUSH,                         // Flushing cache line
    STATE_REFILL,                        // Filling cache line
    STATE_FORWARD,                       // Forward request to directory or L2
  };

  MemoryState           state, previousState;

  bool                  currentlyIdle;

  NetworkFIFO<Word>     inputQueue;      // Input queue
  NetworkFIFO<Word>     inResponseQueue; // Responses from L2 or memory
  DelayFIFO<Word>       outputDataQueue; // Output queue
  DelayFIFO<Word>       outputInstQueue; // Output queue
  DelayFIFO<Word>       outputReqQueue;  // Output request queue
  DelayFIFO<Word>       outputRespQueue; // Output response queue

  // If this bank is flushing data, it has the only valid copy, but the tags
  // will suggest that it doesn't have it at all. Record the addresses of all
  // cache lines in the process of being flushed to detect this corner case.
  vector<MemoryAddr>    pendingFlushes;

  typedef struct {
    MemoryTag address;  // Which data is stored here?
    bool      valid;    // Is the data present?
    bool      dirty;    // Has this line been modified?
    bool      l2Skip;   // Should this line bypass the L2 when flushed?
  } TagData;

  vector<uint32_t>      data;            // The stored data.
  vector<TagData>       metadata;        // Tags, etc. for each cache line.

  // All memory requests that can be in flight at once. To allow hit-under-miss
  // we currently allow a hitting request and a missing request.
  DecodedRequest hitRequest, missRequest;

  ReservationHandler    reservations;    // Data keeping track of current atomic transactions.

  unsigned int          cacheLineCursor; // Used to step through a cache line.

  FIFO<NetworkRequest>  missBuffer; // Payloads for a request which is currently missing.

  bool                  copyingToMissBuffer;   // Tell whether the miss buffer needs filling.
  bool                  readingFromMissBuffer; // Tell whether the miss buffer needs emptying.

  sc_event              cacheMissEvent;  // Event triggered on each cache miss.

  // L2 requests are broadcast to all banks to allow high associativity. This
  // module filters out requests which are for this bank.
  L2RequestFilter       l2RequestFilter;

  // Magic connection to main memory.
  MainMemory *mainMemory;

  // Data received through the L2 request filter.
  RequestSignal requestSig;


};

#endif /* SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYBANK_H_ */
