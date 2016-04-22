/*
 * MainMemory.cpp
 *
 *  Created on: 21 Apr 2016
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "MainMemory.h"
#include "../../Utility/Instrumentation/MainMemory.h"
#include "Operations/MemoryOperationDecode.h"
#include <iomanip>
#include <ios>

using std::dec;
using std::hex;
using std::setfill;
using std::setprecision;

MainMemory::MainMemory(sc_module_name name, ComponentID ID, uint controllers) :
    MemoryInterface(name, ID),
    mux("mux", controllers),
    mData(MAIN_MEMORY_SIZE, 0) {

  assert(controllers >= 1);

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
  for (uint i=0; i<controllers; i++)
    SPAWN_METHOD(mOutputQueues[i]->writeEvent(), MainMemory::sendData, i, true);

}

MainMemory::~MainMemory() {
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
  return (address < mData.size()*BYTES_PER_WORD) && (address == position);
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
  assert(level == MEMORY_OFF_CHIP);
  return muxOutput.valid() && isPayload(muxOutput.read());
}

// Retrieve a payload flit. `level` tells whether this bank is being treated
// as an L1 or L2 cache.
uint32_t MainMemory::getPayload(MemoryLevel level) {
  assert(level == MEMORY_OFF_CHIP);
  NetworkRequest request = muxOutput.read();
  uint32_t payload = request.payload().toUInt();
  currentChannel = mux.getSelection();

  // Continue serving the same input if we haven't reached the end of packet.
  holdMux.write(!request.getMetadata().endOfPacket);
  muxOutput.ack();

  return payload;
}

// Send a result to the requested destination.
void MainMemory::sendResponse(NetworkResponse response, MemoryLevel level) {
  assert(level == MEMORY_OFF_CHIP);
  assert(!mOutputQueues[currentChannel]->full());

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

// Check whether it is safe to write to the given address.
void MainMemory::preWriteCheck(MemoryAddr address) const {
  if (WARN_READ_ONLY && readOnly(address))
    LOKI_WARN << this->name() << " attempting to modify read-only address " << LOKI_HEX(address) << endl;
}

uint32_t MainMemory::readWord(SRAMAddress position) const {
  checkAlignment(position, 4);

  return mData[position/BYTES_PER_WORD];
}

uint32_t MainMemory::readHalfword(SRAMAddress position) const {
  checkAlignment(position, 2);

  uint32_t fullWord = readWord(position & ~0x3);
  uint32_t offset = (position & 0x3) >> 1;
  return (fullWord >> (offset * 16)) & 0xFFFF;
}

uint32_t MainMemory::readByte(SRAMAddress position) const {
  uint32_t fullWord = readWord(position & ~0x3);
  uint32_t offset = position & 0x3UL;
  return (fullWord >> (offset * 8)) & 0xFF;
}

void MainMemory::writeWord(SRAMAddress position, uint32_t data) {
  checkAlignment(position, 4);

  mData[position/BYTES_PER_WORD] = data;
}

void MainMemory::writeHalfword(SRAMAddress position, uint32_t data) {
  checkAlignment(position, 2);

  uint32_t oldData = readWord(position & ~0x3);
  uint32_t offset = (position >> 1) & 0x1;
  uint32_t mask = 0xFFFF << (offset * 16);
  uint32_t newData = (~mask & oldData) | (mask & (data << (16*offset)));

  writeWord(position & ~0x3, newData);
}

void MainMemory::writeByte(SRAMAddress position, uint32_t data) {
  uint32_t oldData = readWord(position & ~0x3);
  uint32_t offset = position & 0x3;
  uint32_t mask = 0xFF << (offset * 8);
  uint32_t newData = (~mask & oldData) | (mask & (data << (8*offset)));

  writeWord(position & ~0x3, newData);
}

bool MainMemory::readOnly(MemoryAddr addr) const {
  for (uint i=0; i<readOnlyBase.size(); i++) {
    if ((addr >= readOnlyBase[i]) && (addr < readOnlyLimit[i])) {
      return true;
    }
  }

  return false;
}

void MainMemory::storeData(vector<Word>& data, MemoryAddr location, bool readOnly) {
  size_t count = data.size();
  uint32_t address = location / 4;

  checkAlignment(location, 4);
  assert(location + count*4 < mData.size());

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
  assert(mState == STATE_IDLE);

  // Check for new requests.
  if (muxOutput.valid()) {
    NetworkRequest request = muxOutput.read();

    // Continue serving the same input if we haven't reached the end of packet.
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
        break;
      case STORE_LINE:
        Instrumentation::MainMemory::write(mActiveRequest->getAddress(), CACHE_LINE_WORDS);
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
  assert(mState == STATE_REQUEST);
  assert(mActiveRequest != NULL);

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
    oData[port].write(response);
    next_trigger(oData[port].ack_event());
  }
}
