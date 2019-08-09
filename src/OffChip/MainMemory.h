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

#ifndef SRC_TILE_MEMORY_MAINMEMORY_H_
#define SRC_TILE_MEMORY_MAINMEMORY_H_

#include "../Memory/MemoryBase.h"
#include "../Utility/LokiVector.h"
#include "MainMemoryRequestHandler.h"

class MemoryOperation;

class MainMemory: public MemoryBase {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput           iClock;

  // One input/output from each memory controller.
  typedef sc_port<network_sink_ifc<Word>> InPort;
  typedef sc_port<network_source_ifc<Word>> OutPort;
  LokiVector<InPort>   iData;
  LokiVector<OutPort>  oData;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  MainMemory(sc_module_name name, uint controllers,
             const main_memory_parameters_t& params);

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
  virtual void makeReservation(ComponentID requester, MemoryAddr address, MemoryAccessMode mode);

  // Return whether a load-linked reservation is still valid.
  virtual bool checkReservation(ComponentID requester, MemoryAddr address, MemoryAccessMode mode) const;

  // Check whether it is safe for the given operation to modify memory.
  virtual void preWriteCheck(const Memory::MemoryOperation& operation) const;

  // Check whether a memory location is read-only.
  bool readOnly(MemoryAddr addr) const;

  // Update coherence information in cases where data doesn't need to be loaded
  // from main memory (e.g. memset).
  void claimCacheLine(ComponentID bank, MemoryAddr address);

  // Store initial program data.
  void storeData(vector<Word>& data, MemoryAddr location, bool readOnly);

  void print(MemoryAddr start, MemoryAddr end) const;


  // Messages between this centralised memory and each of the request handlers.

  // Is a request handler allowed to start a new request? It may not be allowed
  // if the maximum number of requests are already in progress.
  bool canStartRequest() const;

  // Event triggered whenever it is possible to start a new request.
  const sc_event& canStartRequestEvent() const;

  // Tell this memory whenever a request starts or completes.
  void notifyRequestStart();
  void notifyRequestComplete();

protected:

  virtual const vector<uint32_t>& dataArray() const;
  virtual vector<uint32_t>& dataArray();

private:

  // Keep track of which tiles have valid copies of which data for debugging.
  void checkSafeRead(MemoryAddr address, TileID requester);
  void checkSafeWrite(MemoryAddr address, TileID requester);

  Chip& parent() const;

//============================================================================//
// Subcomponents
//============================================================================//

private:

  // One handler per input port.
  LokiVector<MainMemoryRequestHandler> handlers;

  friend class MainMemoryRequestHandler;

//============================================================================//
// Local state
//============================================================================//

private:

  // The stored data.
  vector<uint32_t>   mData;

  // The number of requests in flight at the moment. I use this as a proxy for
  // memory bandwidth being used, with each request using one flit per cycle.
  // This allows us to simulate different bandwidths by capping the number of
  // active requests.
  // I make the assumption that one request will never progress faster than
  // another, giving the same behaviour as a fully sequential model. e.g. If
  // two requests access the same cache line, one will always access a part
  // of the line which has already been processed by the other.
  unsigned int       activeRequests;

  // Mainly for debug, mark the read-only sections of the address space.
  vector<MemoryAddr> readOnlyBase, readOnlyLimit;

  // For debug: record which tiles have an up-to-date copy of each cache line
  // so we can detect incoherent memory use.
  // Each entry in the vector represents a bitmask of all tiles on chip (< 32).
  vector<uint>       cacheLineValid;

  sc_event           bandwidthAvailableEvent;

};

#endif /* SRC_TILE_MEMORY_MAINMEMORY_H_ */
