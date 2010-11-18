/*
 * AddressedWord.h
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef ADDRESSEDWORD_H_
#define ADDRESSEDWORD_H_

#include "Word.h"

class AddressedWord {

//==============================//
// Methods
//==============================//

public:

  Word     payload() const;
  uint16_t channelID() const;
  bool     portClaim() const;
  bool     endOfPacket() const;

  void     notEndOfPacket();

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
  AddressedWord(const Word w, const uint16_t chID, const bool portClaim=false);
  virtual ~AddressedWord();

//==============================//
// Local state
//==============================//

private:

  // The data being transmitted.
  Word payload_;

  // The location the data is being transmitted to.
  uint16_t channelID_;

  // Marks whether or not this is a claim for a port. Once an input port is
  // claimed, all of its responses will be sent back to the sending output port.
  bool portClaim_;

  bool endOfPacket_;

};

#endif /* ADDRESSEDWORD_H_ */
