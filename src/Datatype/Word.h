/*
 * Word.h
 *
 * Defines a common structure for all data types and useful utility methods
 * and operators. Each subclass defines its own way of interpreting the data
 * held in a Word.
 *
 * TODO: a Word is 16 bytes (holds a long and a virtual function table pointer),
 * so allocating huge vectors of Words is very expensive. When storing Words, we
 * should probably only store the integer contents.
 *  * Update: removed all virtual methods, so now down to 8 bytes. Can continue
 *    to 4 bytes when we have a better instruction encoding.
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
  Word(const uint64_t data_);

//==============================//
// Methods
//==============================//

public:

  int32_t  toInt() const;
  int64_t  toLong() const;
  Word     getByte(int byte) const;
  Word     getHalfWord(int half) const;
  Word     setByte(int byte, int newData) const;
  Word     setHalfWord(int half, int newData) const;

  // Returns an integer representing the least significant specified number
  // of bits.
  uint32_t lowestBits(const int limit) const;

  friend void sc_trace(sc_core::sc_trace_file*& tf, const Word& w, const std::string& txt) {
    sc_core::sc_trace(tf, w.data_, txt);
  }

  bool     operator== (const Word& other) const;

  Word&    operator= (const Word& other);

  friend std::ostream& operator<< (std::ostream& os, Word const& v) {
    return v.print(os);
  }

protected:

  uint64_t getBits(const int start, const int end) const;
  void     setBits(const int start, const int end, const uint64_t value);
  void     clearBits(const int start, const int end);

  // Holds the implementation of the << operator, so it does not need to be in
  // the header.
  std::ostream& print(std::ostream& os) const;

//==============================//
// Local state
//==============================//

protected:

  uint64_t data_;

};

#endif /* WORD_H_ */
