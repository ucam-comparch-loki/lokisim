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

  Word     payload() const;
  uint16_t channelID() const;

  friend void sc_trace(sc_core::sc_trace_file*& tf, const AddressedWord& w, const std::string& txt) {
    sc_trace(tf, w.payload_, txt + ".payload");
    sc_core::sc_trace(tf, w.channelID_, txt + ".channelID");
  }

  bool operator== (const AddressedWord& other) const;

  AddressedWord& operator= (const AddressedWord& other);

  friend std::ostream& operator<< (std::ostream& os, AddressedWord const& v) {
    os << "(" << v.payload_ << " -> " << v.channelID_ << ")";
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

  Word payload_;
  uint16_t channelID_;
  // Type of word?

};

#endif /* ADDRESSEDWORD_H_ */
