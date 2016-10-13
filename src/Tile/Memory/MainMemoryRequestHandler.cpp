/*
 * MainMemoryRequestHandler.cpp
 *
 *  Created on: 12 Oct 2016
 *      Author: db434
 */

#include "MainMemoryRequestHandler.h"
#include "MainMemory.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/MainMemory.h"
#include "../../Tile/Memory/Operations/MemoryOperationDecode.h"

MainMemoryRequestHandler::MainMemoryRequestHandler(sc_module_name name, ComponentID ID, MainMemory& memory) :
    MemoryBase(name, ID),
    iClock("iClock"),
    iData("iData"),
    oData("oData"),
    outputQueue("delay", 1024 /* "infinite" size */, (double)MAIN_MEMORY_LATENCY),
    mainMemory(memory) {

  requestState = STATE_IDLE;
  activeRequest = NULL;

  SC_METHOD(mainLoop);
  sensitive << iClock.pos();
  dont_initialize();

  SC_METHOD(sendData);
  sensitive << outputQueue.writeEvent();
  dont_initialize();

}

MainMemoryRequestHandler::~MainMemoryRequestHandler() {
  // Nothing
}


// Compute the position in SRAM that the given memory address is to be found.
SRAMAddress MainMemoryRequestHandler::getPosition(MemoryAddr address, MemoryAccessMode mode) const {
  return mainMemory.getPosition(address, mode);
}

// Return the position in the memory address space of the data stored at the
// given position.
MemoryAddr MainMemoryRequestHandler::getAddress(SRAMAddress position) const {
  return mainMemory.getAddress(position);
}

// Return whether data from `address` can be found at `position` in the SRAM.
bool MainMemoryRequestHandler::contains(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) const {
  return mainMemory.contains(address, position, mode);
}

// Ensure that a valid copy of data from `address` can be found at `position`.
void MainMemoryRequestHandler::allocate(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) {
  mainMemory.allocate(address, position, mode);
}

// Ensure that there is a space to write data to `address` at `position`.
void MainMemoryRequestHandler::validate(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) {
  mainMemory.validate(address, position, mode);
}

// Invalidate the cache line which contains `position`.
void MainMemoryRequestHandler::invalidate(SRAMAddress position, MemoryAccessMode mode) {
  mainMemory.invalidate(position, mode);
}

// Flush the cache line containing `position` down the memory hierarchy, if
// necessary. The line is not invalidated, but is no longer dirty.
void MainMemoryRequestHandler::flush(SRAMAddress position, MemoryAccessMode mode) {
  mainMemory.flush(position, mode);
}

// Return whether a payload flit is available. `level` tells whether this bank
// is being treated as an L1 or L2 cache.
bool MainMemoryRequestHandler::payloadAvailable(MemoryLevel level) const {
  loki_assert_with_message(level == MEMORY_OFF_CHIP, "Level = %d", level);
  return iData.valid() && isPayload(iData.read());
}

// Retrieve a payload flit. `level` tells whether this bank is being treated
// as an L1 or L2 cache.
uint32_t MainMemoryRequestHandler::getPayload(MemoryLevel level) {
  loki_assert_with_message(level == MEMORY_OFF_CHIP, "Level = %d", level);
  NetworkRequest request = iData.read();
  uint32_t payload = request.payload().toUInt();
  Instrumentation::MainMemory::receiveData(request);

  iData.ack();

  return payload;
}

// Send a result to the requested destination.
void MainMemoryRequestHandler::sendResponse(NetworkResponse response, MemoryLevel level) {
  loki_assert_with_message(level == MEMORY_OFF_CHIP, "Level = %d", level);
  loki_assert(!outputQueue.full());

  outputQueue.write(response);
}

// Make a load-linked reservation.
void MainMemoryRequestHandler::makeReservation(ComponentID requester, MemoryAddr address) {
  mainMemory.makeReservation(requester, address);
}

// Return whether a load-linked reservation is still valid.
bool MainMemoryRequestHandler::checkReservation(ComponentID requester, MemoryAddr address) const {
  return mainMemory.checkReservation(requester, address);
}

// Check whether it is safe for the given operation to modify memory.
void MainMemoryRequestHandler::preWriteCheck(const MemoryOperation& operation) const {
  if (WARN_READ_ONLY && mainMemory.readOnly(operation.getAddress())) {
     LOKI_WARN << this->name() << " attempting to modify read-only address" << endl;
     LOKI_WARN << "  " << operation.toString() << endl;
  }
}

const vector<uint32_t>& MainMemoryRequestHandler::dataArrayReadOnly() const {
  return mainMemory.dataArrayReadOnly();
}

vector<uint32_t>& MainMemoryRequestHandler::dataArray() {
  return mainMemory.dataArray();
}

void MainMemoryRequestHandler::mainLoop() {
  switch (requestState) {
    case STATE_IDLE:      processIdle();      break;
    case STATE_REQUEST:   processRequest();   break;
  }
}

void MainMemoryRequestHandler::processIdle() {
  loki_assert_with_message(requestState == STATE_IDLE, "State = %d", requestState);

  // Check with main memory that we are allowed to start a new request.
  if (!mainMemory.canStartRequest()) {
    next_trigger(mainMemory.canStartRequestEvent());
  }
  // Check for new requests.
  else if (iData.valid()) {
    NetworkRequest request = iData.read();
    Instrumentation::MainMemory::receiveData(request);
    iData.ack();

    ChannelID returnAddress(request.getMemoryMetadata().returnTileX,
                            request.getMemoryMetadata().returnTileY,
                            request.getMemoryMetadata().returnChannel,
                            0);

    activeRequest = decodeMemoryRequest(request, *this, MEMORY_OFF_CHIP, returnAddress);

    // Tell main memory that we've started a new request.
    mainMemory.notifyRequestStart();

    // Main memory only supports a subset of operations.
    switch (activeRequest->getMetadata().opcode) {
      case FETCH_LINE:
        Instrumentation::MainMemory::read(activeRequest->getAddress(), CACHE_LINE_WORDS);
        mainMemory.checkSafeRead(activeRequest->getAddress(), returnAddress.component.tile);
        break;
      case STORE_LINE:
        Instrumentation::MainMemory::write(activeRequest->getAddress(), CACHE_LINE_WORDS);
        mainMemory.checkSafeWrite(activeRequest->getAddress(), returnAddress.component.tile);
        break;
      default:
        throw InvalidOptionException("main memory operation", activeRequest->getMetadata().opcode);
        break;
    }

    requestState = STATE_REQUEST;
    next_trigger(sc_core::SC_ZERO_TIME);

    LOKI_LOG << this->name() << " starting " << memoryOpName(activeRequest->getMetadata().opcode)
        << " request from component " << activeRequest->getDestination().component << endl;
  }
  // Nothing to do - wait for input to arrive.
  else {
    next_trigger(iData.default_event());
  }
}

void MainMemoryRequestHandler::processRequest() {
  loki_assert_with_message(requestState == STATE_REQUEST, "State = %d", requestState);
  loki_assert(activeRequest != NULL);

  if (!iClock.negedge()) {
    next_trigger(iClock.negedge_event());
  }
  else if (!activeRequest->preconditionsMet()) {
    activeRequest->prepare();
  }
  else if (!activeRequest->complete()) {
    activeRequest->execute();
  }

  // If the operation has finished, end the request and prepare for a new one.
  if (activeRequest->complete() && requestState == STATE_REQUEST) {
    requestState = STATE_IDLE;
    delete activeRequest;
    activeRequest = NULL;

    // Tell main memory that we've finished a request.
    mainMemory.notifyRequestComplete();

    // Decode the next request immediately so it is ready to start next cycle.
    next_trigger(sc_core::SC_ZERO_TIME);
  }
}

void MainMemoryRequestHandler::sendData() {
  if (outputQueue.empty())
    next_trigger(outputQueue.writeEvent());
  else if (oData.valid()) {
    LOKI_LOG << this->name() << " is blocked waiting for output to become free" << endl;
    next_trigger(oData.ack_event());
  }
  else if (!iClock.posedge())
    next_trigger(iClock.posedge_event());
  else {
    NetworkResponse response = outputQueue.read();
    LOKI_LOG << this->name() << " sending " << response << endl;
    Instrumentation::MainMemory::sendData(response);
    oData.write(response);
    next_trigger(oData.ack_event());
  }
}
