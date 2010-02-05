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

protected:

  unsigned long data;

  unsigned int getBits(int start, int end) const;   // long?
  void setBits(int start, int end, int value);
  void clearBits(int start, int end);

public:

  Word();
  Word(unsigned long data_);
  virtual ~Word();

/* Necessary functions/operators to pass this datatype down a channel */

  friend void sc_trace(sc_core::sc_trace_file*& tf, const Word& w, std::string& txt) {
    sc_core::sc_trace(tf, w.data, txt);
  }

  bool operator== (const Word& other);

  Word& operator= (const Word& other);

  friend std::ostream& operator<< (std::ostream& os, Word const& v) {
    os << v.data;
    return os;
  }

};

#endif /* WORD_H_ */
