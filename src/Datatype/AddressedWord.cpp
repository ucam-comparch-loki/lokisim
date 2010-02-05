/*
 * AddressedWord.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "AddressedWord.h"

Word AddressedWord::getPayload() {
  return payload;
}

int AddressedWord::getAddress() {
  return address;
}

short AddressedWord::getChannelID() {
  return channelID;
}

AddressedWord::AddressedWord() {
  payload = Word();   // Is this stack-safe?
  address = 0;
  channelID = 0;
}

AddressedWord::AddressedWord(Word w, int addr, short id) {
  payload = w;
  address = addr;
  channelID = id;
}

AddressedWord::~AddressedWord() {

}


/* Necessary functions/operators to pass this datatype down a channel */

bool AddressedWord::operator== (const AddressedWord& other) {
  return (this->payload == other.payload)
      && (this->address == other.address)
      && (this->channelID == other.channelID);
}

AddressedWord& AddressedWord::operator= (const AddressedWord& other) {
  payload = other.payload;
  address = other.address;
  channelID = other.channelID;

  return *this;
}
