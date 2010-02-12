/*
 * AddressedWord.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "AddressedWord.h"

Word AddressedWord::getPayload() const {
  return payload;
}

short AddressedWord::getChannelID() const {
  return channelID;
}

AddressedWord::AddressedWord() {
  payload = *(new Word());
  channelID = 0;
}

AddressedWord::AddressedWord(Word w, short id) {
  payload = w;
  channelID = id;
}

AddressedWord::~AddressedWord() {

}


/* Necessary functions/operators to pass this datatype down a channel */

bool AddressedWord::operator== (const AddressedWord& other) const {
  return (this->payload == other.payload)
      && (this->channelID == other.channelID);
}

AddressedWord& AddressedWord::operator= (const AddressedWord& other) {
  payload = other.payload;
  channelID = other.channelID;

  return *this;
}
