/*
 * ALUOperations.h
 *
 * All operations which execute in the ALU.
 *
 * This includes:
 *  * Arithmetic (add, multiply, ...)
 *  * Bitwise operations (and, or, ...)
 *  * Comparisons (setlt, setgte, ...)
 *  * Shifts (sll, srl, ...)
 *
 * In the chain of mix-ins, the ALU operation must be outside any other mix-ins
 * which determine which operands are available.
 *
 * i.e. OtherStuff<Add<Dest2Src<ThreeRegFormat<Instruction>>>>
 *
 *  Created on: 5 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_OPERATIONS_ALU_H_
#define SRC_ISA_OPERATIONS_ALU_H_

#include <cassert>
#include "../../Datatype/Instruction.h"

namespace ISA {

template<class T>
class Add : public Has2Operands<HasResult<T>> {
protected:
  Add(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {
    carryFlag = false;
  }

  void compute() {
    // Not sure why double cast is necessary.
    uint64_t val1 = (uint64_t)((uint32_t)this->operand1);
    uint64_t val2 = (uint64_t)((uint32_t)this->operand2);
    uint64_t result64 = val1 + val2;

    this->result = result64 & 0xFFFFFFFF;
    carryFlag = (result64 >> 32) != 0;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return carryFlag;
  }

  bool carryFlag;
};

template<class T>
class Subtract : public Has2Operands<HasResult<T>> {
protected:
  Subtract(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {}
  void compute() {
    this->result = this->operand1 - this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    // Predicate bit = borrow bit.
    return (this->result < 0);
  }
};

// Count leading zeros.
template<class T>
class CountLeadingZeros : public Has1Operand<HasResult<T>> {
protected:
  CountLeadingZeros(Instruction encoded) : Has1Operand<HasResult<T>>(encoded) {}
  void compute() {
    // Copy any 1-bits into every less-significant position.
    int32_t a = this->operand1;
    a |= (a >> 1);
    a |= (a >> 2);
    a |= (a >> 4);
    a |= (a >> 8);
    a |= (a >> 16);
    this->result = 32 - __builtin_popcount(a);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

// Multiply and return the high word of the result.
template<class T>
class MultiplyHighWord : public Has2Operands<HasResult<T>> {
protected:
  MultiplyHighWord(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {}
  void compute() {
    this->result = ((int64_t)this->operand1 * (int64_t)this->operand2) >> 32;

    // Multiplies take an extra cycle.
    this->finished.notify(1, sc_core::SC_NS);
  }
};

// Multiply and return the high word of the result. Operands are treated as
// unsigned.
template<class T>
class MultiplyHighWordUnsigned : public Has2Operands<HasResult<T>> {
protected:
  MultiplyHighWordUnsigned(Instruction encoded) :
      Has2Operands<HasResult<T>>(encoded) {
    // Nothing
  }
  void compute() {
    // Not sure why double cast is necessary, but it is.
    this->result = ((uint64_t)((uint32_t)this->operand1) *
                    (uint64_t)((uint32_t)this->operand2)) >> 32;

    // Multiplies take an extra cycle.
    this->finished.notify(1, sc_core::SC_NS);
  }
};

// Multiply and return the low word of the result.
template<class T>
class MultiplyLowWord : public Has2Operands<HasResult<T>> {
protected:
  MultiplyLowWord(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {}
  void compute() {
    this->result = ((int64_t)this->operand1 *
                    (int64_t)this->operand2) & 0xFFFFFFFF;

    // Multiplies take an extra cycle.
    this->finished.notify(1, sc_core::SC_NS);
  }
};

// Select an input based on the predicate.
template<class T>
class PredicatedSelect : public ReadPredicate<Has2Operands<HasResult<T>>> {
protected:
  PredicatedSelect(Instruction encoded) :
      ReadPredicate<Has2Operands<HasResult<T>>>(encoded) {
    // Nothing
  }
  void compute() {
    this->result = this->predicateBit ? this->operand1 : this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};


template<class T>
class BitwiseAnd : public Has2Operands<HasResult<T>> {
protected:
  BitwiseAnd(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {}
  void compute() {
    this->result = this->operand1 & this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};

template<class T>
class BitwiseOr : public Has2Operands<HasResult<T>> {
protected:
  BitwiseOr(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {}
  void compute() {
    this->result = this->operand1 | this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};

template<class T>
class BitwiseNor : public Has2Operands<HasResult<T>> {
protected:
  BitwiseNor(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {}
  void compute() {
    this->result = ~(this->operand1 | this->operand2);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};

template<class T>
class BitwiseXor : public Has2Operands<HasResult<T>> {
protected:
  BitwiseXor(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {}
  void compute() {
    this->result = this->operand1 ^ this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};


template<class T>
class ShiftLeftLogical : public Has2Operands<HasResult<T>> {
protected:
  ShiftLeftLogical(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {}
  void compute() {
    this->result = this->operand1 << this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

template<class T>
class ShiftRightLogical : public Has2Operands<HasResult<T>> {
protected:
  ShiftRightLogical(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {
    // Nothing
  }
  void compute() {
    this->result = (uint32_t)this->operand1 >> this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};

template<class T>
class ShiftRightArithmetic : public Has2Operands<HasResult<T>> {
protected:
  ShiftRightArithmetic(Instruction encoded) :
      Has2Operands<HasResult<T>>(encoded) {
    // Nothing
  }
  void compute() {
    this->result = this->operand1 >> this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};


template<class T>
class SetIfEqual : public Has2Operands<HasResult<T>> {
protected:
  SetIfEqual(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {}
  void compute() {
    this->result = this->operand1 == this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};

template<class T>
class SetIfNotEqual : public Has2Operands<HasResult<T>> {
protected:
  SetIfNotEqual(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {}
  void compute() {
    this->result = this->operand1 != this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};

template<class T>
class SetIfLessThan : public Has2Operands<HasResult<T>> {
protected:
  SetIfLessThan(Instruction encoded) : Has2Operands<HasResult<T>>(encoded) {}
  void compute() {
    this->result = this->operand1 < this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};

template<class T>
class SetIfLessThanUnsigned : public Has2Operands<HasResult<T>> {
protected:
  SetIfLessThanUnsigned(Instruction encoded) :
      Has2Operands<HasResult<T>>(encoded) {
    // Nothing
  }
  void compute() {
    this->result = ((uint32_t)this->operand1 < (uint32_t)this->operand2);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};

template<class T>
class SetIfGreaterThanOrEqual : public Has2Operands<HasResult<T>> {
protected:
  SetIfGreaterThanOrEqual(Instruction encoded) :
      Has2Operands<HasResult<T>>(encoded) {
    // Nothing
  }
  void compute() {
    this->result = this->operand1 >= this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};

template<class T>
class SetIfGreaterThanOrEqualUnsigned : public Has2Operands<HasResult<T>> {
protected:
  SetIfGreaterThanOrEqualUnsigned(Instruction encoded) :
      Has2Operands<HasResult<T>>(encoded) {
    // Nothing
  }
  void compute() {
    this->result = ((uint32_t)this->operand1 >= (uint32_t)this->operand2);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  bool computePredicate() {
    return this->result & 1;
  }
};

} // end namespace

#endif /* SRC_ISA_OPERATIONS_ALU_H_ */
