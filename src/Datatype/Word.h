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

#include "../Utility/Parameters.h"

class Word {
protected:
	uint64_t data_;

	/* Return the integer value of the bits between the start and end positions,
	* inclusive. */
	inline uint64_t getBits(int start, int end) const {
		uint64_t result = (end>=63) ? data_ : data_ % (1L << (end+1));
		result = result >> start;
		return result;
	}

	/* Replace the specified bit range by the specified value */
	inline void setBits(int start, int end, uint64_t value) {
	  // Is this any better?
	  // uint64_t newval = value << start;
	  // uint64_t mask   = ((1L << (end - start + 1)) - 1) << start;
	  // data_           = (data_ & ~mask) | (newval & mask);

		// Ensure that value only sets bits in the specified range
		unsigned long maskedValue = value % (1L << (end - start + 1));
		// Clear any existing bits in this word
		clearBits(start, end);
		// Combine this word with the value
		data_ |= maskedValue << start;
	}

	/* Zero the bits between the start and end positions, inclusive. */
	inline void clearBits(int start, int end) {
		uint64_t mask  = 1L << (end - start + 1);         // 0000100000
		mask -= 1;                                        // 0000011111
		mask = mask << start;                             // 0011111000
		mask = ~mask;                                     // 1100000111
		data_ &= mask;
	}
public:
	inline int32_t toInt() const		{return (int32_t)data_;}
	inline uint32_t toUInt() const		{return (uint32_t)data_;}
	inline int64_t toLong() const		{return (int64_t)data_;}
	inline uint64_t toULong() const		{return (uint64_t)data_;}

	inline Word getByte(int byte) const {
		assert(byte < BYTES_PER_WORD && byte >= 0);
		int value = getBits(byte*8, byte*8 + 7);
		return Word(value);
	}

	inline Word getHalfWord(int half) const {
		assert(half < 2 && half >= 0);
		int value = getBits(half*16, half*16 + 15);
		return Word(value);
	}

	inline Word setByte(int byte, int newData) const {
		assert(byte < BYTES_PER_WORD && byte >= 0);
		Word newWord(*this);
		newWord.setBits(byte*8, byte*8 + 7, newData);
		return newWord;
	}

	inline Word setHalfWord(int half, int newData) const {
		assert(half < 2 && half >= 0);
		Word newWord(*this);
		newWord.setBits(half*16, half*16 + 15, newData);
		return newWord;
	}

	/* Used to extract some bits and put them in the indirection registers. */

	inline uint32_t lowestBits(int limit) const {
		return getBits(0, limit-1);
	}

	friend void sc_trace(sc_core::sc_trace_file*& tf, const Word& w, const std::string& txt) {
		sc_core::sc_trace(tf, w.data_, txt);
	}

	/* Necessary functions/operators to pass this datatype down a channel */

	inline bool operator== (const Word& other) const	{return this->data_ == other.data_;}
	inline bool operator!= (const Word& other) const	{return this->data_ != other.data_;}
	inline bool operator< (const Word& other) const		{return this->data_ < other.data_;}
	inline bool operator<= (const Word& other) const	{return this->data_ <= other.data_;}
	inline bool operator> (const Word& other) const		{return this->data_ > other.data_;}
	inline bool operator>= (const Word& other) const	{return this->data_ >= other.data_;}

	inline Word& operator= (const Word& other) {
		data_ = other.data_;
		return *this;
	}

	friend std::ostream& operator<< (std::ostream& os, Word const& v) {
		// Instructions are the only datatypes capable of using more than 32 bits,
		// and they are also the ones which are most useful to print differently.

		/*
		if ((v.data_ >> 32) && (v.data_ >> 32 != 0xFFFFFFFF))
			os << static_cast<Instruction>(v);
		else
			os << v.toLong();
		*/

		os << v.toLong();
		return os;
	}

	Word() {
		data_ = 0;
	}

	Word(const Word& other) {
		data_ = other.data_;
	}

	Word(uint64_t data_) : data_(data_) {
		// Do nothing
	}
};

#endif /* WORD_H_ */
