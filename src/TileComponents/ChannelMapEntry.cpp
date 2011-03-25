/*
 * ChannelMapEntry.cpp
 *
 *  Created on: 14 Mar 2011
 *      Author: db434
 */

#include "ChannelMapEntry.h"
#include <iostream>

#define MAX_CREDITS CHANNEL_END_BUFFER_SIZE

ChannelID ChannelMapEntry::destination() const {
  return destination_;
}

int ChannelMapEntry::network() const {
  return network_;
}

bool ChannelMapEntry::canSend() const {
  return !useCredits_ || (credits_ > 0);
}

bool ChannelMapEntry::haveAllCredits() const {
  return credits_ == MAX_CREDITS;
}

void ChannelMapEntry::destination(ChannelID address) {
  // Only allow the destination to change when all credits from previous
  // destination have been received.
  assert(credits_ == MAX_CREDITS);

  destination_ = address;

  // For now:
  // TODO: determine which network to use.
  network_ = 0;
  useCredits_ = true;
}

void ChannelMapEntry::removeCredit() {
  credits_--;
  assert(credits_ >= 0 && credits_ <= MAX_CREDITS);
}

void ChannelMapEntry::addCredit() {
  credits_++;
  assert(credits_ >= 0 && credits_ <= MAX_CREDITS);
}

ChannelMapEntry& ChannelMapEntry::operator=(ChannelID address) {
  destination(address);
  return *this;
}

ChannelMapEntry::ChannelMapEntry() {
  destination_ = 0xFFFFFFFF;
  credits_ = MAX_CREDITS;
}
