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

#include <inttypes.h>
#include "systemc"

class Word {

//==============================//
// Constructors and destructors
//==============================//

public:

  Word();
  explicit Word(const uint64_t data_);
  virtual ~Word();

//==============================//
// Methods
//==============================//

public:

  int32_t toInt() const;
  int64_t toLong() const;

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

  uint64_t getBits(const int start, const int end) const;
  void     setBits(const int start, const int end, const uint64_t value);
  void     clearBits(const int start, const int end);

//==============================//
// Local state
//==============================//

protected:

  uint64_t data;

};

#endif /* WORD_H_ */
