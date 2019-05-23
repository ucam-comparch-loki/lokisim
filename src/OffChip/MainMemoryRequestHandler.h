/*
 * MainMemoryRequestHandler.h
 *
 * Component responsible for responding to requests on one input port of
 * main memory.
 *
 *  Created on: 12 Oct 2016
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_MAINMEMORYREQUESTHANDLER_H_
#define SRC_TILE_MEMORY_MAINMEMORYREQUESTHANDLER_H_

#include "../Memory/MemoryBase.h"
#include <memory>
#include "../Network/FIFOs/DelayFIFO.h"

class MainMemory;

class MainMemoryRequestHandler: public MemoryBase {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput     iClock;

  sc_port<network_sink_ifc<Word>>   iData;
  sc_port<network_source_ifc<Word>> oData;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(MainMemoryRequestHandler);
  MainMemoryRequestHandler(sc_module_name name, MainMemory& memory,
                           const main_memory_parameters_t& params);
  virtual ~MainMemoryRequestHandler();

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
  virtual void preWriteCheck(const MemoryOperation& operation) const;

protected:

  virtual const vector<uint32_t>& dataArray() const;
  virtual vector<uint32_t>& dataArray();

private:

  void mainLoop();
  void processIdle();
  void processRequest();

  // Instrumentation method triggered whenever data is sent.
  void sentData();

//============================================================================//
// Local state
//============================================================================//

private:

  enum RequestHandlerState {
    STATE_IDLE,                        // No active request
    STATE_REQUEST,                     // Serving active request
  };

  RequestHandlerState   requestState;
  std::unique_ptr<MemoryOperation> activeRequest; // The request being served.

  NetworkFIFO<Word>     inputQueue;
  DelayFIFO<Word>       outputQueue; // Model memory latency

  // The place where data is actually stored. There may be many of these
  // request handlers all accessing the same data.
  MainMemory&           mainMemory;

};

#endif /* SRC_TILE_MEMORY_MAINMEMORYREQUESTHANDLER_H_ */
