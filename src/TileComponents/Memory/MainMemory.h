/*
 * MainMemory.h
 *
 * Main memory model.
 *
 * Accepts one request at a time, and only supports FETCH_LINE and STORE_LINE
 * operations.
 *
 *  Created on: 21 Apr 2016
 *      Author: db434
 */

#ifndef SRC_TILECOMPONENTS_MEMORY_MAINMEMORY_H_
#define SRC_TILECOMPONENTS_MEMORY_MAINMEMORY_H_

#include "../../Component.h"
#include "../../Network/ArbitratedMultiplexer.h"
#include "../../Network/DelayBuffer.h"
#include "MemoryBase.h"

class MemoryOperation;

class MainMemory: public MemoryBase {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput                 iClock;

  // One input/output from each memory controller.
  LokiVector<RequestInput>   iData;
  LokiVector<ResponseOutput> oData;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(MainMemory);
  MainMemory(sc_module_name name, ComponentID ID, uint controllers);
  virtual ~MainMemory();

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

  // Check whether it is safe to write to the given address.
  virtual void preWriteCheck(MemoryAddr address) const;

  // Check whether a memory location is read-only.
  bool readOnly(MemoryAddr addr) const;

  // Store initial program data.
  void storeData(vector<Word>& data, MemoryAddr location, bool readOnly);

  void print(MemoryAddr start, MemoryAddr end) const;

private:

  void mainLoop();
  void processIdle();
  void processRequest();

  // Method triggered whenever data needs to be sent.
  void sendData(uint port);

  // Keep track of which tiles have valid copies of which data for debugging.
  void checkSafeRead(MemoryAddr address, TileID requester);
  void checkSafeWrite(MemoryAddr address, TileID requester);

//============================================================================//
// Subcomponents
//============================================================================//

private:

  // Choose which input to serve next.
  ArbitratedMultiplexer<NetworkRequest> mux;
  loki_signal<NetworkRequest>           muxOutput;
  sc_signal<bool>                       holdMux;    // Continue reading from same input

//============================================================================//
// Local state
//============================================================================//

private:

  enum MainMemoryState {
    STATE_IDLE,                        // No active request
    STATE_REQUEST,                     // Serving active request
  };

  MainMemoryState    mState;
  MemoryOperation*   mActiveRequest;   // The request being served.

  vector<DelayBuffer<NetworkResponse>*> mOutputQueues; // Model memory latency
  PortIndex          currentChannel;   // The input currently being served.


  // Mainly for debug, mark the read-only sections of the address space.
  vector<MemoryAddr> readOnlyBase, readOnlyLimit;

  // For debug: record which tiles have an up-to-date copy of each cache line
  // so we can detect incoherent memory use.
  // Each entry in the vector represents a bitmask of all tiles on chip (< 32).
  vector<uint>       cacheLineValid;

};

#endif /* SRC_TILECOMPONENTS_MEMORY_MAINMEMORY_H_ */
