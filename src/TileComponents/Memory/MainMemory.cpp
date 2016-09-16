/*
 * MainMemory.cpp
 *
 *  Created on: 21 Apr 2016
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "MainMemory.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/MainMemory.h"
#include "Operations/MemoryOperationDecode.h"
#include <iomanip>
#include <ios>

using std::dec;
using std::hex;
using std::setfill;
using std::setprecision;

MainMemory::MainMemory(sc_module_name name, ComponentID ID, uint controllers) :
    MemoryBase(name, ID, MAIN_MEMORY_SIZE),
    mux("mux", controllers),
    cacheLineValid(MAIN_MEMORY_SIZE / CACHE_LINE_BYTES, 0) {

  loki_assert(controllers >= 1);

  iData.init(controllers);
  oData.init(controllers);

  mux.iData(iData);
  mux.oData(muxOutput);
  mux.iHold(holdMux);
  holdMux.write(false);

  for(uint i=0; i<controllers; i++) {
    mOutputQueues.push_back(
        new DelayBuffer<NetworkResponse>(sc_gen_unique_name("outQueue"),
                                         1024, // "Infinite" size.
                                         (double)MAIN_MEMORY_LATENCY)
    );
  }

  currentChannel = 0;

  SC_METHOD(mainLoop);
  sensitive << iClock.pos();
  dont_initialize();

  // Methods for returning data to each controller.
  for (uint i=0; i<mOutputQueues.size(); i++)
    SPAWN_METHOD(mOutputQueues[i]->writeEvent(), MainMemory::sendData, i, true);

}

MainMemory::~MainMemory() {
  for (uint i=0; i<mOutputQueues.size(); i++)
    delete mOutputQueues[i];
}

// Compute the position in SRAM that the given memory address is to be found.
SRAMAddress MainMemory::getPosition(MemoryAddr address, MemoryAccessMode mode) const {
  // Main memory acts like a giant scratchpad - there is no address
  // transformation.
  return (SRAMAddress)address;
}

// Return the position in the memory address space of the data stored at the
// given position.
MemoryAddr MainMemory::getAddress(SRAMAddress position) const {
  // Main memory acts like a giant scratchpad - there is no address
  // transformation.
  return (MemoryAddr)position;
}

// Return whether data from `address` can be found at `position` in the SRAM.
bool MainMemory::contains(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) const {
  loki_assert_with_message(address < mData.size()*BYTES_PER_WORD, "Address 0x%x", address);
  loki_assert(address == position);
  return true;
}

// Ensure that a valid copy of data from `address` can be found at `position`.
void MainMemory::allocate(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) {
  // Do nothing.
}

// Ensure that there is a space to write data to `address` at `position`.
void MainMemory::validate(MemoryAddr address, SRAMAddress position, MemoryAccessMode mode) {
  // Do nothing.
}

// Invalidate the cache line which contains `position`.
void MainMemory::invalidate(SRAMAddress position, MemoryAccessMode mode) {
  // Do nothing.
}

// Flush the cache line containing `position` down the memory hierarchy, if
// necessary. The line is not invalidated, but is no longer dirty.
void MainMemory::flush(SRAMAddress position, MemoryAccessMode mode) {
  // Do nothing.
}

// Return whether a payload flit is available. `level` tells whether this bank
// is being treated as an L1 or L2 cache.
bool MainMemory::payloadAvailable(MemoryLevel level) const {
  loki_assert_with_message(level == MEMORY_OFF_CHIP, "Level = %d", level);
  return muxOutput.valid() && isPayload(muxOutput.read());
}

// Retrieve a payload flit. `level` tells whether this bank is being treated
// as an L1 or L2 cache.
uint32_t MainMemory::getPayload(MemoryLevel level) {
  loki_assert_with_message(level == MEMORY_OFF_CHIP, "Level = %d", level);
  NetworkRequest request = muxOutput.read();
  uint32_t payload = request.payload().toUInt();
  Instrumentation::MainMemory::receiveData(request);

  // Continue serving the same input if we haven't reached the end of packet.
  holdMux.write(!request.getMetadata().endOfPacket);
  muxOutput.ack();

  return payload;
}

// Send a result to the requested destination.
void MainMemory::sendResponse(NetworkResponse response, MemoryLevel level) {
  loki_assert_with_message(level == MEMORY_OFF_CHIP, "Level = %d", level);
  loki_assert(!mOutputQueues[currentChannel]->full());

  mOutputQueues[currentChannel]->write(response);
}

// Make a load-linked reservation.
void MainMemory::makeReservation(ComponentID requester, MemoryAddr address) {
  // Do nothing.
}

// Return whether a load-linked reservation is still valid.
bool MainMemory::checkReservation(ComponentID requester, MemoryAddr address) const {
  // Do nothing.
  return false;
}

// Check whether it is safe for the given operation to modify memory.
void MainMemory::preWriteCheck(const MemoryOperation& operation) const {
  if (WARN_READ_ONLY && readOnly(operation.getAddress())) {
     LOKI_WARN << this->name() << " attempting to modify read-only address" << endl;
     LOKI_WARN << "  " << operation.toString() << endl;
  }
}

bool MainMemory::readOnly(MemoryAddr addr) const {
  for (uint i=0; i<readOnlyBase.size(); i++) {
    if ((addr >= readOnlyBase[i]) && (addr < readOnlyLimit[i])) {
      return true;
    }
  }

  return false;
}

void MainMemory::claimCacheLine(ComponentID bank, MemoryAddr address) {
  int tile = bank.tile.computeTileIndex();
  int cacheLine = getLine(address);

  // This is now the only tile with an up-to-date copy of the data.
  cacheLineValid[cacheLine] = (1 << tile);
}

void MainMemory::storeData(vector<Word>& data, MemoryAddr location, bool readOnly) {
  size_t count = data.size();
  uint32_t address = location / 4;

  checkAlignment(location, 4);
  loki_assert_with_message(location + count*4 < mData.size(), "Upper limit = 0x%x", location + count*4);

  for (size_t i = 0; i < count; i++) {
    LOKI_LOG << this->name() << " wrote to " << LOKI_HEX((address+i)*4) << ": " << data[i].toUInt() << endl;

    mData[address + i] = data[i].toUInt();
  }

  if (readOnly) {
    readOnlyBase.push_back(location);
    readOnlyLimit.push_back(location + 4*data.size());
  }
}

void MainMemory::print(MemoryAddr start, MemoryAddr end) const {
  if (start > end) {
    MemoryAddr temp = start;
    start = end;
    end = temp;
  }

  size_t address = start / 4;
  size_t limit = end / 4 + 1;

  cout << "Main memory data:\n" << endl;

  while (address < limit) {
    assert(address < mData.size());
    cout << "0x" << setprecision(8) << setfill('0') << hex << (address * 4)
         << ":  " << "0x" << setprecision(8) << setfill('0') << hex << mData[address] << dec << endl;
    address++;
  }
}

void MainMemory::mainLoop() {
  switch (mState) {
    case STATE_IDLE:      processIdle();      break;
    case STATE_REQUEST:   processRequest();   break;
  }
}

void MainMemory::processIdle() {
  loki_assert_with_message(mState == STATE_IDLE, "State = %d", mState);

  // Check for new requests.
  if (muxOutput.valid()) {
    NetworkRequest request = muxOutput.read();
    Instrumentation::MainMemory::receiveData(request);

    // Continue serving the same input if we haven't reached the end of packet.
    currentChannel = mux.getSelection();
    holdMux.write(!request.getMetadata().endOfPacket);
    muxOutput.ack();

    ChannelID returnAddress(request.getMemoryMetadata().returnTileX,
                            request.getMemoryMetadata().returnTileY,
                            request.getMemoryMetadata().returnChannel,
                            0);

    mActiveRequest = decodeMemoryRequest(request, *this, MEMORY_OFF_CHIP, returnAddress);

    // Main memory only supports a subset of operations.
    switch (mActiveRequest->getMetadata().opcode) {
      case FETCH_LINE:
        Instrumentation::MainMemory::read(mActiveRequest->getAddress(), CACHE_LINE_WORDS);
        checkSafeRead(mActiveRequest->getAddress(), returnAddress.component.tile);
        break;
      case STORE_LINE:
        Instrumentation::MainMemory::write(mActiveRequest->getAddress(), CACHE_LINE_WORDS);
        checkSafeWrite(mActiveRequest->getAddress(), returnAddress.component.tile);
        break;
      default:
        throw InvalidOptionException("main memory operation", mActiveRequest->getMetadata().opcode);
        break;
    }

    mState = STATE_REQUEST;
    next_trigger(sc_core::SC_ZERO_TIME);

    LOKI_LOG << this->name() << " starting " << memoryOpName(mActiveRequest->getMetadata().opcode)
        << " request from component " << mActiveRequest->getDestination().component << endl;
  }
  // Nothing to do - wait for input to arrive.
  else {
    next_trigger(muxOutput.default_event());
  }
}

void MainMemory::processRequest() {
  loki_assert_with_message(mState == STATE_REQUEST, "State = %d", mState);
  loki_assert(mActiveRequest != NULL);

  if (!iClock.negedge()) {
    next_trigger(iClock.negedge_event());
  }
  else if (!mActiveRequest->preconditionsMet()) {
    mActiveRequest->prepare();
  }
  else if (!mActiveRequest->complete()) {
    mActiveRequest->execute();
  }

  // If the operation has finished, end the request and prepare for a new one.
  if (mActiveRequest->complete() && mState == STATE_REQUEST) {
    mState = STATE_IDLE;
    delete mActiveRequest;
    mActiveRequest = NULL;

    // Decode the next request immediately so it is ready to start next cycle.
    next_trigger(sc_core::SC_ZERO_TIME);
  }
}

void MainMemory::sendData(uint port) {
  if (mOutputQueues[port]->empty())
    next_trigger(mOutputQueues[port]->writeEvent());
  else if (oData[port].valid()) {
    LOKI_LOG << this->name() << " is blocked waiting for output to become free" << endl;
    next_trigger(oData[port].ack_event());
  }
  else if (!iClock.posedge())
    next_trigger(iClock.posedge_event());
  else {
    NetworkResponse response = mOutputQueues[port]->read();
    LOKI_LOG << this->name() << " sending " << response << endl;
    Instrumentation::MainMemory::sendData(response);
    oData[port].write(response);
    next_trigger(oData[port].ack_event());
  }
}

void MainMemory::checkSafeRead(MemoryAddr address, TileID requester) {
  uint tile = requester.computeTileIndex();
  uint cacheLine = getLine(address);

  loki_assert_with_message(cacheLine < cacheLineValid.size(), "Address = 0x%x", address);

  // After reading, this tile has an up-to-date copy of the data.
  cacheLineValid[cacheLine] |= (1 << tile);
}

void MainMemory::checkSafeWrite(MemoryAddr address, TileID requester) {
  uint tile = requester.computeTileIndex();
  uint cacheLine = getLine(address);

  loki_assert_with_message(cacheLine < cacheLineValid.size(), "Address = 0x%x", address);

  if (WARN_INCOHERENCE && !(cacheLineValid[cacheLine] & (1 << tile)))
    LOKI_WARN << "Tile " << requester << " updated cache line "
    << LOKI_HEX(address) << " without first having an up-to-date copy." << endl;

  // After writing, this is the only tile with an up-to-date copy of the data.
  cacheLineValid[cacheLine] = (1 << tile);
}
