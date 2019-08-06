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

template<class T>
class Add : public T {
protected:
  Add(Instruction encoded) : T(encoded) {carryFlag = false;}
  void compute() {
    // Not sure why double cast is necessary.
    uint64_t val1 = (uint64_t)((uint32_t)this->operand1);
    uint64_t val2 = (uint64_t)((uint32_t)this->operand2);
    uint64_t result64 = val1 + val2;

    this->result = result64 & 0xFFFFFFFF;
    carryFlag = (result64 >> 32) != 0;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  void computePredicate() {
    this->newPredicate = carryFlag;
  }

  bool carryFlag;
};

template<class T>
class Subtract : public T {
protected:
  Subtract(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 - this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  void computePredicate() {
    // Predicate bit = borrow bit.
    this->newPredicate = (this->result < 0);
  }
};

// Count leading zeros.
template<class T>
class CLZ : public T {
protected:
  CLZ(Instruction encoded) : T(encoded) {}
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
class MulHW : public T {
protected:
  MulHW(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = ((int64_t)this->operand1 * (int64_t)this->operand2) >> 32;

    // Multiplies take an extra cycle.
    this->finished.notify(1, sc_core::SC_NS);
  }
};

// Multiply and return the high word of the result. Operands are treated as
// unsigned.
template<class T>
class MulHWU : public T {
protected:
  MulHWU(Instruction encoded) : T(encoded) {}
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
class MulLW : public T {
protected:
  MulLW(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = ((int64_t)this->operand1 *
                    (int64_t)this->operand2) & 0xFFFFFFFF;

    // Multiplies take an extra cycle.
    this->finished.notify(1, sc_core::SC_NS);
  }
};

// Select an input based on the predicate.
template<class T>
class PSel : public T {
protected:
  PSel(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->predicateBit ? this->operand1 : this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};


template<class T>
class BitwiseAnd : public T {
protected:
  BitwiseAnd(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 & this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

template<class T>
class BitwiseOr : public T {
protected:
  BitwiseOr(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 | this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

template<class T>
class BitwiseNor : public T {
protected:
  BitwiseNor(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = ~(this->operand1 | this->operand2);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

template<class T>
class BitwiseXor : public T {
protected:
  BitwiseXor(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 ^ this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};


template<class T>
class ShiftLeftLogical : public T {
protected:
  ShiftLeftLogical(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 << this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

template<class T>
class ShiftRightLogical : public T {
protected:
  ShiftRightLogical(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = (uint32_t)this->operand1 >> this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

template<class T>
class ShiftRightArithmetic : public T {
protected:
  ShiftRightArithmetic(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 >> this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};


template<class T>
class SetEq : public T {
protected:
  SetEq(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 == this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  void computePredicate() {
    this->newPredicate = this->result & 1;
  }
};

template<class T>
class SetNE : public T {
protected:
  SetNE(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 != this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  void computePredicate() {
    this->newPredicate = this->result & 1;
  }
};

template<class T>
class SetLT : public T {
protected:
  SetLT(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 < this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  void computePredicate() {
    this->newPredicate = this->result & 1;
  }
};

template<class T>
class SetLTU : public T {
protected:
  SetLTU(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = ((uint32_t)this->operand1 < (uint32_t)this->operand2);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  void computePredicate() {
    this->newPredicate = this->result & 1;
  }
};

template<class T>
class SetGTE : public T {
protected:
  SetGTE(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 >= this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  void computePredicate() {
    this->newPredicate = this->result & 1;
  }
};

template<class T>
class SetGTEU : public T {
protected:
  SetGTEU(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = ((uint32_t)this->operand1 >= (uint32_t)this->operand2);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
  void computePredicate() {
    this->newPredicate = this->result & 1;
  }
};


#endif /* SRC_ISA_OPERATIONS_ALU_H_ */
