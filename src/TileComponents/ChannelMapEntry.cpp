/*
 * ChannelMapEntry.cpp
 *
 *  Created on: 14 Mar 2011
 *      Author: db434
 */

#include "ChannelMapEntry.h"
#include "../Datatype/Identifier.h"
#include "../Exceptions/InvalidOptionException.h"
#include "../Utility/Arguments.h"
#include <iostream>

using sc_core::sc_event;

#define MAX_CREDITS IN_CHANNEL_BUFFER_SIZE

ChannelID ChannelMapEntry::getDestination() const {
  switch (network_) {
    case CORE_TO_CORE:
      return ChannelID(getCoreMask(), getChannel());
    case CORE_TO_MEMORY:
      return ChannelID(getTileColumn(), getTileRow(), getComponent()+CORES_PER_TILE, getChannel());
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

bool ChannelMapEntry::writeThrough() const {
  return writeThrough_;
}

bool ChannelMapEntry::canSend() const {
  return !useCredits_ || (credits_ > 0);
}

bool ChannelMapEntry::haveAllCredits() const {
  return !useCredits_ || (credits_ == MAX_CREDITS);
}

// Write to the channel map entry.
void ChannelMapEntry::write(EncodedCMTEntry data) {
  // Only allow the destination to change when all credits from previous
  // destination have been received.
  assert(haveAllCredits());

  data_ = data;

  if (isMulticast()) {
    useCredits_ = false;
    network_ = CORE_TO_CORE;

    if (DEBUG || Arguments::summarise())
      std::cout << "SETCHMAP: " << id_ << " -> " << getDestination() << std::endl;
  }
  else if ((getTileColumn() != id_.component.tile.x) || (getTileRow() != id_.component.tile.y)) {
    useCredits_ = true;
    network_ = GLOBAL;

    if (DEBUG || Arguments::summarise())
      std::cout << "SETCHMAP: " << id_ << " -> " << getDestination() << std::endl;
  }
  else {
    useCredits_ = false;
    network_ = CORE_TO_MEMORY;

    // Print a description of the configuration so we can more-easily check that
    // memory is being accessed correctly.
    if (DEBUG || Arguments::summarise()) {
      string type = (getReturnChannel() < 2) ? "insts" : "data";
      uint startBank = getComponent();
      uint endBank = startBank + getMemoryGroupSize() - 1;
      uint lineSize = getLineSize();

      std::cout << "SETCHMAP: core " << id_.component << " " << type << ", banks "
          << startBank << "-" << endBank << ", line size " << lineSize << std::endl;
    }
  }
}

// Get the entire contents of the channel map entry (used for the getchmap
// instruction).
EncodedCMTEntry ChannelMapEntry::read() const {
  return data_;
}

unsigned int ChannelMapEntry::getTileColumn() const {
  assert(!isMulticast());
  uint mask = (1 << TILE_X_WIDTH) - 1;
  return (data_ >> TILE_X_START) & mask;
}

unsigned int ChannelMapEntry::getTileRow() const {
  assert(!isMulticast());
  uint mask = (1 << TILE_Y_WIDTH) - 1;
  return (data_ >> TILE_Y_START) & mask;
}

// The position of the target component within its tile.
ComponentIndex ChannelMapEntry::getComponent() const {
  assert(!isMulticast());
  uint mask = (1 << POSITION_WIDTH) - 1;
  return (data_ >> POSITION_START) & mask;
}

// The channel of the target component to access.
ChannelIndex ChannelMapEntry::getChannel() const {
  uint mask = (1 << CHANNEL_WIDTH) - 1;
  return (data_ >> CHANNEL_START) & mask;
}

// Whether this is a multicast communication.
bool ChannelMapEntry::isMulticast() const {
  uint mask = (1 << MULTICAST_WIDTH) - 1;
  return (data_ >> MULTICAST_START) & mask;
}

// A bitmask of cores in the local tile to communicate with. The same channel
// on all cores is used.
unsigned int ChannelMapEntry::getCoreMask() const {
  assert(isMulticast());
  uint mask = (1 << COREMASK_WIDTH) - 1;
  return (data_ >> COREMASK_START) & mask;
}

// The number of memory banks in the virtual memory group.
unsigned int ChannelMapEntry::getMemoryGroupSize() const {
  uint mask = (1 << GROUP_SIZE_WIDTH) - 1;
  uint bits = (data_ >> GROUP_SIZE_START) & mask;
  return 1 << bits;
}

// The number of words in each cache line.
unsigned int ChannelMapEntry::getLineSize() const {
  uint mask = (1 << LINE_SIZE_WIDTH) - 1;
  uint bits = (data_ >> LINE_SIZE_START) & mask;

  switch ((LineSizeWords)bits) {
    case LS_4: return 4;
    case LS_8: return 8;
    case LS_16: return 16;
    case LS_32: return 32;
    default:
      assert(false);
      return -1;
  }
}

// The input channel on this core to which any requested data should be sent
// from memory.
ChannelIndex ChannelMapEntry::getReturnChannel() const {
  uint mask = (1 << RETURN_CHANNEL_WIDTH) - 1;
  return (data_ >> RETURN_CHANNEL_START) & mask;
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

void ChannelMapEntry::removeCredit() {
  if (!useCredits_)
    return;

  credits_--;
  assert(credits_ >= 0);
  assert(credits_ <= MAX_CREDITS);
}

void ChannelMapEntry::addCredit() {
  if (!useCredits_)
    return;

  credits_++;
  assert(credits_ >= 0);
  assert(credits_ <= MAX_CREDITS);

  creditArrived_.notify();

  if (credits_ == MAX_CREDITS)
    haveAllCredits_.notify();
}

bool ChannelMapEntry::usesCredits() const {
  return useCredits_;
}

uint ChannelMapEntry::popCount() const {
  return __builtin_popcount(data_);
}

uint ChannelMapEntry::hammingDistance(const ChannelMapEntry& other) const {
  return __builtin_popcount(data_ ^ other.data_);
}

const sc_event& ChannelMapEntry::allCreditsEvent() const {
  return haveAllCredits_;
}

const sc_event& ChannelMapEntry::creditArrivedEvent() const {
  return creditArrived_;
}

ChannelMapEntry::ChannelMapEntry(ChannelID localID) :
  id_(localID),
  data_(0),
  network_(CORE_TO_CORE),
  useCredits_(false),
  credits_(MAX_CREDITS),
  writeThrough_(false),
  addressIncrement_(0) {
}

ChannelMapEntry::ChannelMapEntry(const ChannelMapEntry& other) :
  id_(other.id_),
  data_(other.data_),
  network_(other.network_),
  useCredits_(other.useCredits_),
  credits_(other.credits_),
  writeThrough_(other.writeThrough_),
  addressIncrement_(other.addressIncrement_) {
}

ChannelMapEntry& ChannelMapEntry::operator=(const ChannelMapEntry& other) {
  id_ = other.id_;
  data_ = other.data_;
  credits_ = other.credits_;
  useCredits_ = other.useCredits_;
  addressIncrement_ = other.addressIncrement_;

  network_ = other.network_;
  writeThrough_ = other.writeThrough_;

  return *this;
}
