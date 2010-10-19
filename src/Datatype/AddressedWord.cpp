/*
 * AddressedWord.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "../Utility/Parameters.h"
#include "AddressedWord.h"

Word AddressedWord::payload() const {
  return payload_;
}

uint16_t AddressedWord::channelID() const {
  return channelID_;
}

bool AddressedWord::portClaim() const {
  return portClaim_;
}

AddressedWord::AddressedWord() {
  payload_ = *(new Word());
  channelID_ = 0;
}

AddressedWord::AddressedWord(const Word w, const uint16_t id, const bool portClaim) {
  payload_ = w;
  channelID_ = id;
  portClaim_ = portClaim;

  if((int)id < 0 || (int)id > NUM_TILES*COMPONENTS_PER_TILE*NUM_CLUSTER_OUTPUTS) {
    std::cerr << "Warning: planning to send to channel " << (int)id << std::endl;
  }
}

AddressedWord::~AddressedWord() {

}


/* Necessary functions/operators to pass this datatype down a channel */

bool AddressedWord::operator== (const AddressedWord& other) const {
  return (this->payload_ == other.payload_)
      && (this->channelID_ == other.channelID_)
      && (this->portClaim_ == other.portClaim_);
}

AddressedWord& AddressedWord::operator= (const AddressedWord& other) {
  payload_ = other.payload_;
  channelID_ = other.channelID_;
  portClaim_ = other.portClaim_;

  return *this;
}
