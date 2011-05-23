/*
 * ChannelMapEntry.cpp
 *
 *  Created on: 14 Mar 2011
 *      Author: db434
 */

#include "ChannelMapEntry.h"
#include "../Datatype/ChannelID.h"
#include <iostream>

#define MAX_CREDITS CHANNEL_END_BUFFER_SIZE

ChannelID ChannelMapEntry::destination() const {
  return destination_;
}

ChannelMapEntry::NetworkType ChannelMapEntry::network() const {
  return network_;
}

bool ChannelMapEntry::localMemory() const {
	return localMemory_;
}

int ChannelMapEntry::memoryGroupBits() const {
	return memoryGroupBits_;
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
	assert(!useCredits_ || (credits_ == MAX_CREDITS));

	destination_ = address;
	network_ = /*(address.getTile() == local) ?*/ CORE_TO_CORE /*: GLOBAL*/;
	useCredits_ = true;
	localMemory_ = false;
	memoryGroupBits_ = 0;
	addressIncrement_ = 0;
}

void ChannelMapEntry::setMemoryDestination(const ChannelID& address, uint memoryGroupBits) {
	// Only allow the destination to change when all credits from previous
	// destination have been received.
	assert(!useCredits_ || (credits_ == MAX_CREDITS));

	destination_ = address;
	network_ = CORE_TO_MEMORY;
	useCredits_ = false;
	localMemory_ = true;
	memoryGroupBits_ = memoryGroupBits;
	addressIncrement_ = 0;
}

void ChannelMapEntry::setAddressIncrement(uint increment) {
	addressIncrement_ = increment;
}

uint ChannelMapEntry::getAddressIncrement() {
	return addressIncrement_;
}

void ChannelMapEntry::removeCredit() {
	if (!useCredits_)
		return;

	credits_--;
	assert(credits_ >= 0 && credits_ <= MAX_CREDITS);
}

void ChannelMapEntry::addCredit() {
	if (!useCredits_)
		return;

	credits_++;
	assert(credits_ >= 0 && credits_ <= MAX_CREDITS);
}

ChannelMapEntry::ChannelMapEntry() {
  destination_ = ChannelID();
  credits_ = MAX_CREDITS;
  useCredits_ = false;
  localMemory_ = false;
  memoryGroupBits_ = 0;
  addressIncrement_ = 0;
}
