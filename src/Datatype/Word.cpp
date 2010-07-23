/*
 * Word.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "Word.h"

/* Return the integer value of the bits between the start and end positions,
   inclusive. */
unsigned int Word::getBits(int start, int end) const {
  unsigned long result = (end>=63) ? data : data % (1L << (end+1));
  result = result >> start;
  return (int)result;
}

/* Replace the specified bit range by the specified value */
void Word::setBits(int start, int end, int value) {
  // Ensure that value only sets bits in the specified range
  unsigned long maskedValue = value % (1L << (end - start + 1));
  // Clear any existing bits in this word
  clearBits(start, end);
  // Combine this word with the value
  data |= maskedValue << start;
}

/* Zero the bits between the start and end positions, inclusive. */
void Word::clearBits(int start, int end) {
  unsigned long mask  = 1L << (end - start + 1);    // 0000100000
  mask -= 1;                                        // 0000011111
  mask = mask << start;                             // 0011111000
  mask = ~mask;                                     // 1100000111
  data &= mask;
}

/* Constructors and destructors */
Word::Word() {
  data = 0;
}

Word::Word(unsigned long data_) : data(data_) {
  // Do nothing
}

Word::~Word() {

}

/* Necessary functions/operators to pass this datatype down a channel */

bool Word::operator== (const Word& other) const {
  return this->data == other.data;
}

Word& Word::operator= (const Word& other) {
  data = other.data;

  return *this;
}
