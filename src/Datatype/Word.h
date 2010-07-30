/*
 * Word.h
 *
 * Defines a common structure for all data types and useful utility methods
 * and operators. Each subclass defines its own way of interpreting the data
 * held in a Word.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef WORD_H_
#define WORD_H_

#include "systemc"

class Word {

//==============================//
// Constructors and destructors
//==============================//

public:

  Word();
  explicit Word(unsigned long data_);
  virtual ~Word();

//==============================//
// Methods
//==============================//

public:

  friend void sc_trace(sc_core::sc_trace_file*& tf, const Word& w, const std::string& txt) {
    sc_core::sc_trace(tf, w.data, txt);
  }

  bool operator== (const Word& other) const;

  Word& operator= (const Word& other);

  friend std::ostream& operator<< (std::ostream& os, Word const& v) {
    os << (int)v.data;
    return os;
  }

protected:

  unsigned int getBits(int start, int end) const;   // long?
  void setBits(int start, int end, int value);
  void clearBits(int start, int end);

//==============================//
// Local state
//==============================//

protected:

  unsigned long data;

};

#endif /* WORD_H_ */
