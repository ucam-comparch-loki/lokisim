/*
 * Word.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "Word.h"
#include "Instruction.h"
#include "../Utility/Parameters.h"

int32_t Word::toInt() const {
  return (int32_t)data_;
}

int64_t Word::toLong() const {
  return (int64_t)data_;
}

Word Word::getByte(int byte) const {
  assert(byte < BYTES_PER_WORD && byte >= 0);
  int value = getBits(byte*8, byte*8 + 7);
  return Word(value);
}

Word Word::getHalfWord(int half) const {
  assert(half < 2 && half >= 0);
  int value = getBits(half*16, half*16 + 15);
  return Word(value);
}

Word Word::setByte(int byte, int newData) const {
  assert(byte < BYTES_PER_WORD && byte >= 0);
  Word newWord(*this);
  newWord.setBits(byte*8, byte*8 + 7, newData);
  return newWord;
}

Word Word::setHalfWord(int half, int newData) const {
  assert(half < 2 && half >= 0);
  Word newWord(*this);
  newWord.setBits(half*16, half*16 + 15, newData);
  return newWord;
}

/* Used to extract some bits and put them in the indirection registers. */
uint32_t Word::lowestBits(const int limit) const {
  return getBits(0, limit-1);
}

/* Return the integer value of the bits between the start and end positions,
 * inclusive. */
uint64_t Word::getBits(const int start, const int end) const {
  uint64_t result = (end>=63) ? data_ : data_ % (1L << (end+1));
  result = result >> start;
  return result;
}

/* Replace the specified bit range by the specified value */
void Word::setBits(const int start, const int end, const uint64_t value) {
  // Ensure that value only sets bits in the specified range
  unsigned long maskedValue = value % (1L << (end - start + 1));
  // Clear any existing bits in this word
  clearBits(start, end);
  // Combine this word with the value
  data_ |= maskedValue << start;
}

/* Zero the bits between the start and end positions, inclusive. */
void Word::clearBits(const int start, const int end) {
  uint64_t mask  = 1L << (end - start + 1);         // 0000100000
  mask -= 1;                                        // 0000011111
  mask = mask << start;                             // 0011111000
  mask = ~mask;                                     // 1100000111
  data_ &= mask;
}

std::ostream& Word::print(std::ostream& os) const {
  // Instructions are the only datatypes capable of using more than 32 bits,
  // and they are also the ones which are most useful to print differently.
  if(data_ >> 32) {
    os << static_cast<Instruction>(*this);
  }
  else os << toLong();
  return os;
}

/* Constructors and destructors */
Word::Word() {
  data_ = 0;
}

Word::Word(const uint64_t data_) : data_(data_) {
  // Do nothing
}

Word::~Word() {

}

/* Necessary functions/operators to pass this datatype down a channel */

bool Word::operator== (const Word& other) const {
  return this->data_ == other.data_;
}

Word& Word::operator= (const Word& other) {
  data_ = other.data_;

  return *this;
}
