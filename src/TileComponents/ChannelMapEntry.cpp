/*
 * ChannelMapEntry.cpp
 *
 *  Created on: 14 Mar 2011
 *      Author: db434
 */

#include "ChannelMapEntry.h"
#include "../Datatype/ChannelID.h"
#include <iostream>

using sc_core::sc_event;

#define MAX_CREDITS IN_CHANNEL_BUFFER_SIZE

ChannelID ChannelMapEntry::destination() const {
  return destination_;
}

ChannelMapEntry::NetworkType ChannelMapEntry::network() const {
  return network_;
}

bool ChannelMapEntry::localMemory() const {
  return localMemory_;
}

uint ChannelMapEntry::memoryGroupBits() const {
  return memoryGroupBits_;
}

uint ChannelMapEntry::memoryLineBits() const {
  return memoryLineBits_;
}

bool ChannelMapEntry::writeThrough() const {
  return writeThrough_;
}

ChannelIndex ChannelMapEntry::returnChannel() const {
  return returnChannel_;
}

bool ChannelMapEntry::canSend() const {
  return !useCredits_ || (credits_ > 0);
}

bool ChannelMapEntry::haveAllCredits() const {
  return !useCredits_ || (credits_ == MAX_CREDITS);
}

void ChannelMapEntry::setCoreDestination(const ChannelID& address) {
  // Only allow the destination to change when all credits from previous
  // destination have been received.
  assert(haveAllCredits());

  destination_ = address;
  network_ = (address.getTile() == id_.getTile()) ? CORE_TO_CORE : GLOBAL;
  useCredits_ = network_ == GLOBAL;
  localMemory_ = false;
  returnChannel_ = 0;
  memoryGroupBits_ = 0;
  memoryLineBits_ = 0;
  addressIncrement_ = 0;
}

void ChannelMapEntry::setMemoryDestination(const ChannelID& address,
                                           uint memoryGroupBits,
                                           uint memoryLineBits,
                                           ChannelIndex returnTo,
                                           bool writeThrough) {
  // Only allow the destination to change when all credits from previous
  // destination have been received.
  assert(haveAllCredits());
  assert(address.getTile() == id_.getTile());

  destination_ = address;
  network_ = CORE_TO_MEMORY;
  useCredits_ = false;
  localMemory_ = true;
  returnChannel_ = returnTo;
  memoryGroupBits_ = memoryGroupBits;
  memoryLineBits_ = memoryLineBits;
  writeThrough_ = writeThrough;
  addressIncrement_ = 0;
}

uint ChannelMapEntry::computeAddressIncrement(MemoryAddr address) const {
  uint increment = address;
  increment &= (1UL << (memoryGroupBits() + memoryLineBits())) - 1UL;
  increment >>= memoryLineBits();
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
  return __builtin_popcount(destination_.toInt()) +
         __builtin_popcount(memoryGroupBits_) +
         __builtin_popcount(memoryLineBits_);
}

uint ChannelMapEntry::hammingDistance(const ChannelMapEntry& other) const {
  return __builtin_popcount(destination_.toInt() ^ other.destination_.toInt()) +
         __builtin_popcount(memoryGroupBits_ ^ other.memoryGroupBits_) +
         __builtin_popcount(memoryLineBits_ ^ other.memoryLineBits_);
}

const sc_event& ChannelMapEntry::allCreditsEvent() const {
  return haveAllCredits_;
}

const sc_event& ChannelMapEntry::creditArrivedEvent() const {
  return creditArrived_;
}

ChannelMapEntry::ChannelMapEntry(ComponentID localID) {
  id_ = localID;
  destination_ = ChannelID();
  credits_ = MAX_CREDITS;
  useCredits_ = false;
  localMemory_ = false;
  memoryGroupBits_ = 0;
  memoryLineBits_ = 0;
  addressIncrement_ = 0;

  network_ = CORE_TO_CORE;
  writeThrough_ = false;
  returnChannel_ = -1;
}

ChannelMapEntry::ChannelMapEntry(const ChannelMapEntry& other) {
  id_ = other.id_;
  destination_ = other.destination_;
  credits_ = other.credits_;
  useCredits_ = other.useCredits_;
  localMemory_ = other.localMemory_;
  memoryGroupBits_ = other.memoryGroupBits_;
  memoryLineBits_ = other.memoryLineBits_;
  addressIncrement_ = other.addressIncrement_;

  network_ = other.network_;
  writeThrough_ = other.writeThrough_;
  returnChannel_ = other.returnChannel_;
}

ChannelMapEntry& ChannelMapEntry::operator=(const ChannelMapEntry& other) {
  id_ = other.id_;
  destination_ = other.destination_;
  credits_ = other.credits_;
  useCredits_ = other.useCredits_;
  localMemory_ = other.localMemory_;
  memoryGroupBits_ = other.memoryGroupBits_;
  memoryLineBits_ = other.memoryLineBits_;
  addressIncrement_ = other.addressIncrement_;

  network_ = other.network_;
  writeThrough_ = other.writeThrough_;
  returnChannel_ = other.returnChannel_;

  return *this;
}
