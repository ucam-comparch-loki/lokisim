/*
 * Word.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "Word.h"

int32_t Word::toInt() const {
  return (int32_t)data;
}

int64_t Word::toLong() const {
  return (int64_t)data;
}

/* Return the integer value of the bits between the start and end positions,
   inclusive. */
uint64_t Word::getBits(const int start, const int end) const {
  uint64_t result = (end>=63) ? data : data % (1L << (end+1));
  result = result >> start;
  return (uint64_t)result;
}

/* Replace the specified bit range by the specified value */
void Word::setBits(const int start, const int end, const uint64_t value) {
  // Ensure that value only sets bits in the specified range
  unsigned long maskedValue = value % (1L << (end - start + 1));
  // Clear any existing bits in this word
  clearBits(start, end);
  // Combine this word with the value
  data |= maskedValue << start;
}

/* Zero the bits between the start and end positions, inclusive. */
void Word::clearBits(const int start, const int end) {
  uint64_t mask  = 1L << (end - start + 1);         // 0000100000
  mask -= 1;                                        // 0000011111
  mask = mask << start;                             // 0011111000
  mask = ~mask;                                     // 1100000111
  data &= mask;
}

/* Constructors and destructors */
Word::Word() {
  data = 0;
}

Word::Word(const uint64_t data_) : data(data_) {
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
