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

//==============================//
// Methods
//==============================//

public:

  Word getPayload() const;
  uint16_t getChannelID() const;

  friend void sc_trace(sc_core::sc_trace_file*& tf, const AddressedWord& w, const std::string& txt) {
    sc_trace(tf, w.payload, txt + ".payload");
    sc_core::sc_trace(tf, w.channelID, txt + ".channelID");
  }

  bool operator== (const AddressedWord& other) const;

  AddressedWord& operator= (const AddressedWord& other);

  friend std::ostream& operator<< (std::ostream& os, AddressedWord const& v) {
    os << "(" << v.payload << " -> " << v.channelID << ")";
    return os;
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  AddressedWord();
  AddressedWord(const Word w, const uint16_t chID);
  virtual ~AddressedWord();

//==============================//
// Local state
//==============================//

private:

  Word payload;
  uint16_t channelID;
  // Type of word?

};

#endif /* ADDRESSEDWORD_H_ */
