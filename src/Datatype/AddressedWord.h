/*
 * AddressedWord.h
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef ADDRESSEDWORD_H_
#define ADDRESSEDWORD_H_

#include "Word.h"

class AddressedWord : public Word {

  Word payload;
  int address;
  short channelID;
  // Type of word?

public:
  Word getPayload();
  int getAddress();
  short getChannelID();

  AddressedWord();
  AddressedWord(Word w, int addr, short chID);  // Remove addr?
  virtual ~AddressedWord();


/* Necessary functions/operators to pass this datatype down a channel */

  friend void sc_trace(sc_core::sc_trace_file*& tf, const AddressedWord& w, std::string& txt) {
    //sc_core::sc_trace(tf, w.payload, txt);
    sc_core::sc_trace(tf, &(w.payload), txt);
    sc_core::sc_trace(tf, w.address, txt);
    sc_core::sc_trace(tf, w.channelID, txt);
  }

  bool operator== (const AddressedWord& other);

  AddressedWord& operator= (const AddressedWord& other);

  friend std::ostream& operator<< (std::ostream& os, AddressedWord const& v) {
    os << "(" << v.payload << ", " << v.address << ", " << v.channelID << ")";
    return os;
  }
};

#endif /* ADDRESSEDWORD_H_ */
