/*
 * ChannelMapEntry.cpp
 *
 *  Created on: 14 Mar 2011
 *      Author: db434
 */

#include "../Tile/ChannelMapEntry.h"

#include "../Datatype/Identifier.h"
#include "../Exceptions/InvalidOptionException.h"
#include "../Utility/Arguments.h"
#include "../Utility/Logging.h"
#include <iostream>

using sc_core::sc_event;

const ChannelMapEntry::MulticastChannel ChannelMapEntry::multicastView(EncodedCMTEntry data) {
  return MulticastChannel(data);
}

const ChannelMapEntry::MemoryChannel ChannelMapEntry::memoryView(EncodedCMTEntry data) {
  return MemoryChannel(data);
}

const ChannelMapEntry::GlobalChannel ChannelMapEntry::globalView(EncodedCMTEntry data) {
  return GlobalChannel(data);
}

const ChannelMapEntry::MulticastChannel ChannelMapEntry::multicastView() const {
  return MulticastChannel(data_);
}

const ChannelMapEntry::MemoryChannel ChannelMapEntry::memoryView() const {
  return MemoryChannel(data_);
}

const ChannelMapEntry::GlobalChannel ChannelMapEntry::globalView() const {
  return GlobalChannel(data_);
}

ChannelID ChannelMapEntry::getDestination() const {
  switch (getNetwork()) {
    case MULTICAST:
      return ChannelID(getCoreMask(), getChannel());
    case CORE_TO_MEMORY:
      return ChannelID(id_.component.tile.x, id_.component.tile.y, getComponent(), getChannel());
    case GLOBAL:
      return ChannelID(getTileColumn(), getTileRow(), getComponent(), getChannel());
    default:
      throw InvalidOptionException("channel map entry network", network_);
      break;
  }
}

ChannelMapEntry::NetworkType ChannelMapEntry::getNetwork() const {
  return network_;
}

bool ChannelMapEntry::useCredits() const {
  // Only start counting credits once the connection has been established.
  return getNetwork() == GLOBAL;// && globalView().acquired;
}

bool ChannelMapEntry::canSend() const {
  return !useCredits() || (numCredits() > 0);
}

bool ChannelMapEntry::haveNCredits(uint n) const {
  return !useCredits() || (numCredits() >= n);
}

uint ChannelMapEntry::numCredits() const {
  return globalView().credits;
}

void ChannelMapEntry::removeCredit() {
  if (useCredits()) {
    assert(numCredits() > 0);
    incrementCredits(-1);
  }
}

void ChannelMapEntry::addCredits(uint credits) {
  // If we're using credits, increment the credit counter, as normal.
  if (useCredits() && globalView().acquired) {
    incrementCredits(credits);
    assert(numCredits() > 0);

    creditArrived_.notify();
  }
  // If we're not using credits, this may be an ack/nack from a connection
  // we're trying to set up.
  else if (getNetwork() == GLOBAL) {
    incrementCredits(1);
    setAcquired(credits == 1);
    creditArrived_.notify();
  }
}

void ChannelMapEntry::setCredits(uint count) {
  GlobalChannel entry = globalView();
  entry.credits = count;

  data_ = entry.flatten();
}

void ChannelMapEntry::incrementCredits(int count) {
  // Never increment/decrement the counter if there are infinite credits.
  if (numCredits() != INFINITE_CREDIT_COUNT) {
    setCredits(numCredits() + count);
  }
}

void ChannelMapEntry::clearWriteEnable() {
  GlobalChannel entry = globalView();
  entry.creditWriteEnable = 0;

  data_ = entry.flatten();
}

void ChannelMapEntry::setAcquired(bool acq) {
  GlobalChannel entry = globalView();
  entry.acquired = acq;

  data_ = entry.flatten();
}

// Write to the channel map entry.
void ChannelMapEntry::write(EncodedCMTEntry data) {
  if (data == 0)
    LOKI_WARN << "setting channel map entry " << id_ << " to 0." << std::endl;

  uint oldCredits = numCredits();

  data_ = data;

  if (globalView().isGlobal) {
    network_ = GLOBAL;

    // If the write-enable bit was not set, replace the credit counter with
    // the previous value.
    if (!globalView().creditWriteEnable)
      setCredits(oldCredits);

    clearWriteEnable();
  }
  else if (memoryView().isMemory) {
    network_ = CORE_TO_MEMORY;
  }
  else {
    network_ = MULTICAST;
  }
}

// Get the entire contents of the channel map entry (used for the getchmap
// instruction).
EncodedCMTEntry ChannelMapEntry::read() const {
  return data_;
}

unsigned int ChannelMapEntry::getTileColumn() const {
  assert(getNetwork() == GLOBAL);
  return globalView().tileX;
}

unsigned int ChannelMapEntry::getTileRow() const {
  assert(getNetwork() == GLOBAL);
  return globalView().tileY;
}

// The position of the target component within its tile.
ComponentIndex ChannelMapEntry::getComponent() const {
  if (getNetwork() == CORE_TO_MEMORY)
    return memoryView().bank + CORES_PER_TILE;
  else if (getNetwork() == GLOBAL)
    return globalView().core;
  else {
    assert(false);
    return -1;
  }
}

// The channel of the target component to access.
ChannelIndex ChannelMapEntry::getChannel() const {
  return globalView().channel;
}

// Whether this is a multicast communication.
bool ChannelMapEntry::isMulticast() const {
  return getNetwork() == MULTICAST;
}

bool ChannelMapEntry::isCore() const {
  // This may not be a multicast channel, but converting to one exposes fields
  // which are shared between all channel types.
  MulticastChannel channel = multicastView();
  return channel.isGlobal || !channel.isMemory;
}

bool ChannelMapEntry::isMemory() const {
  // This may not be a multicast channel, but converting to one exposes fields
  // which are shared between all channel types.
  MulticastChannel channel = multicastView();
  return !channel.isGlobal && channel.isMemory;
}

bool ChannelMapEntry::isCore(EncodedCMTEntry data) {
  // This may not be a multicast channel, but converting to one exposes fields
  // which are shared between all channel types.
  MulticastChannel channel = multicastView(data);
  return channel.isGlobal || !channel.isMemory;
}

bool ChannelMapEntry::isMemory(EncodedCMTEntry data) {
  // This may not be a multicast channel, but converting to one exposes fields
  // which are shared between all channel types.
  MulticastChannel channel = multicastView(data);
  return !channel.isGlobal && channel.isMemory;
}

// A bitmask of cores in the local tile to communicate with. The same channel
// on all cores is used.
unsigned int ChannelMapEntry::getCoreMask() const {
  assert(getNetwork() == MULTICAST);
  return multicastView().coreMask;
}

// The number of memory banks in the virtual memory group.
unsigned int ChannelMapEntry::getMemoryGroupSize() const {
  if (getNetwork() == CORE_TO_MEMORY)
    return 1 << memoryView().groupSize;
  else
    return 1;
}

// The number of words in each cache line.
unsigned int ChannelMapEntry::getLineSize() const {
  return CACHE_LINE_WORDS;
}

// The input channel on this core to which any requested data should be sent
// from memory.
ChannelIndex ChannelMapEntry::getReturnChannel() const {
  assert(getNetwork() == CORE_TO_MEMORY);
  return memoryView().returnChannel;
}

uint ChannelMapEntry::computeAddressIncrement(MemoryAddr address) const {
  uint increment = address;
  increment &= (getMemoryGroupSize() * getLineSize() * BYTES_PER_WORD) - 1;
  increment /= getLineSize() * BYTES_PER_WORD;
  return increment;
}

void ChannelMapEntry::setAddressIncrement(uint increment) {
  addressIncrement_ = increment;
}

uint ChannelMapEntry::getAddressIncrement() const {
  return addressIncrement_;
}

uint ChannelMapEntry::popCount() const {
  return __builtin_popcount(read());
}

uint ChannelMapEntry::hammingDistance(const ChannelMapEntry& other) const {
  return __builtin_popcount(read() ^ other.read());
}

const sc_event& ChannelMapEntry::creditArrivedEvent() const {
  return creditArrived_;
}

ChannelMapEntry::ChannelMapEntry(ChannelID localID) :
  id_(localID),
  data_(0),
  network_(MULTICAST),
  addressIncrement_(0) {

}

ChannelMapEntry::ChannelMapEntry(const ChannelMapEntry& other) :
  id_(other.id_),
  data_(other.data_),
  network_(other.network_),
  addressIncrement_(other.addressIncrement_) {

}

ChannelMapEntry& ChannelMapEntry::operator=(const ChannelMapEntry& other) {
  id_ = other.id_;
  data_ = other.data_;
  addressIncrement_ = other.addressIncrement_;

  network_ = other.network_;

  return *this;
}
