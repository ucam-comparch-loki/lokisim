/*
 * MainMemory.cpp
 *
 *  Created on: 21 Apr 2016
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "MainMemory.h"
#include "../Chip.h"
#include "../Memory/MemoryOperations/MemoryOperationDecode.h"
#include "../Utility/Assert.h"
#include "../Utility/Instrumentation/MainMemory.h"
#include <iomanip>
#include <ios>

using std::dec;
using std::hex;
using std::setfill;
using std::setprecision;

MainMemory::MainMemory(sc_module_name name, uint controllers,
                       const main_memory_parameters_t& params) :
    MemoryBase(name, ComponentID(0,0,0), params.log2CacheLineSize()),
    iClock("iClock"),
    iData("iData", controllers),
    oData("oData", controllers),
    mData(params.size/BYTES_PER_WORD, 0),
    cacheLineValid(params.size / params.cacheLineSize, 0) {

  loki_assert(controllers >= 1);

  for (uint i=0; i<controllers; i++) {
    MainMemoryRequestHandler* handler =
        new MainMemoryRequestHandler(sc_gen_unique_name("handler"), *this, params);

    handler->iClock(iClock);
    iData[i](handler->iData);
    oData[i](handler->oData);

    handlers.push_back(handler);
  }

  activeRequests = 0;

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
  loki_assert_with_message(address < dataArray().size()*BYTES_PER_WORD, "Address 0x%x", address);
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
  loki_assert(false);
  return false;
}

// Retrieve a payload flit. `level` tells whether this bank is being treated
// as an L1 or L2 cache.
uint32_t MainMemory::getPayload(MemoryLevel level) {
  loki_assert(false);
  return 0;
}

// Send a result to the requested destination.
void MainMemory::sendResponse(NetworkResponse response, MemoryLevel level) {
  loki_assert(false);
}

// Make a load-linked reservation.
void MainMemory::makeReservation(ComponentID requester, MemoryAddr address, MemoryAccessMode mode) {
  // Do nothing.
}

// Return whether a load-linked reservation is still valid.
bool MainMemory::checkReservation(ComponentID requester, MemoryAddr address, MemoryAccessMode mode) const {
  // Do nothing.
  return false;
}

// Check whether it is safe for the given operation to modify memory.
void MainMemory::preWriteCheck(const Memory::MemoryOperation& operation) const {
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
  uint tile = parent().computeTileIndex(bank.tile);
  uint cacheLine = MemoryBase::getLine(address);

  loki_assert_with_message(cacheLine < cacheLineValid.size(), "Address = 0x%x", address);

  // This is now the only tile with an up-to-date copy of the data.
  cacheLineValid[cacheLine] = (1 << tile);
}

void MainMemory::storeData(vector<Word>& data, MemoryAddr location, bool readOnly) {
  size_t count = data.size();
  uint32_t address = location / BYTES_PER_WORD;

  checkAlignment(location, BYTES_PER_WORD);
  loki_assert_with_message(location + count*BYTES_PER_WORD < mData.size()*BYTES_PER_WORD, "Upper limit = 0x%x", location + count*BYTES_PER_WORD);

  for (size_t i = 0; i < count; i++) {
    LOKI_LOG(3) << this->name() << " wrote to " << LOKI_HEX((address+i)*BYTES_PER_WORD) << ": " << data[i].toUInt() << endl;

    loki_assert(address+i < mData.size());
    mData[address + i] = data[i].toUInt();
  }

  if (readOnly) {
    readOnlyBase.push_back(location);
    readOnlyLimit.push_back(location + count*BYTES_PER_WORD);
  }
}

void MainMemory::print(MemoryAddr start, MemoryAddr end) const {
  if (start > end) {
    MemoryAddr temp = start;
    start = end;
    end = temp;
  }

  size_t address = start / BYTES_PER_WORD;
  size_t limit = end / BYTES_PER_WORD + 1;

  cout << "Main memory data:\n" << endl;

  while (address < limit) {
    assert(address < mData.size());
    cout << "0x" << setprecision(8) << setfill('0') << hex << (address * BYTES_PER_WORD)
         << ":  " << "0x" << setprecision(8) << setfill('0') << hex << mData[address] << dec << endl;
    address++;
  }
}

bool MainMemory::canStartRequest() const {
  return activeRequests < oData.size();
}

const sc_event& MainMemory::canStartRequestEvent() const {
  return bandwidthAvailableEvent;
}

void MainMemory::notifyRequestStart() {
  assert(canStartRequest());
  activeRequests++;
}

void MainMemory::notifyRequestComplete() {
  activeRequests--;
  bandwidthAvailableEvent.notify(sc_core::SC_ZERO_TIME);
}

const vector<uint32_t>& MainMemory::dataArray() const {
  return mData;
}

vector<uint32_t>& MainMemory::dataArray() {
  return mData;
}

void MainMemory::checkSafeRead(MemoryAddr address, TileID requester) {
  uint tile = parent().computeTileIndex(requester);
  uint cacheLine = MemoryBase::getLine(address);

  loki_assert_with_message(cacheLine < cacheLineValid.size(), "Address = 0x%x", address);

  // After reading, this tile has an up-to-date copy of the data.
  cacheLineValid[cacheLine] |= (1 << tile);
}

void MainMemory::checkSafeWrite(MemoryAddr address, TileID requester) {
  uint tile = parent().computeTileIndex(requester);
  uint cacheLine = MemoryBase::getLine(address);

  loki_assert_with_message(cacheLine < cacheLineValid.size(), "Address = 0x%x", address);

  if (WARN_INCOHERENCE && !(cacheLineValid[cacheLine] & (1 << tile)))
    LOKI_WARN << "Tile " << requester << " overwrote cache line "
    << LOKI_HEX(address) << " in main memory using potentially stale data." << endl;

  // After writing, this is the only tile with an up-to-date copy of the data.
  cacheLineValid[cacheLine] = (1 << tile);
}

Chip& MainMemory::parent() const {
  Chip* ptr = static_cast<Chip*>(this->get_parent_object());
  return *ptr;
}
