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
 *  Created on: 5 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_OPERATIONS_ALUOPERATIONS_H_
#define SRC_ISA_OPERATIONS_ALUOPERATIONS_H_

#include <cassert>

// Base class for all ALU operations.
class ALUOperation : public InstructionBase {
protected:
  ALUOperation(Instruction encoded) : InstructionBase(encoded) {}

  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {
    assert(false);
    return 0;
  }
};


class Add : public ALUOperation {
protected:
  Add(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return a+b;}
};

class Subtract : public ALUOperation {
protected:
  Subtract(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return a-b;}
};

// Count leading zeros.
class CLZ : public ALUOperation {
protected:
  CLZ(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {
    // Copy any 1-bits into every less-significant position.
    a |= (a >> 1);
    a |= (a >> 2);
    a |= (a >> 4);
    a |= (a >> 8);
    a |= (a >> 16);
    return 32 - __builtin_popcount(a);
  }
};

// Multiply and return the high word of the result.
class MulHW : public ALUOperation {
protected:
  MulHW(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {
    return ((int64_t)a * (int64_t)b) >> 32;
  }
};

// Multiply and return the high word of the result. Operands are treated as
// unsigned.
class MulHWU : public ALUOperation {
protected:
  MulHWU(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {
    // Not sure why double cast is necessary, but it is.
    return ((uint64_t)((uint32_t)a) * (uint64_t)((uint32_t)b)) >> 32;
  }
};

// Multiply and return the low word of the result.
class MulLW : public ALUOperation {
protected:
  MulLW(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {
    return ((int64_t)a * (int64_t)b) & 0xFFFFFFFF;
  }
};

// Select an input based on the predicate.
class PSel : public ALUOperation {
protected:
  PSel(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {
    return p ? a : b;
  }
};


class BitwiseAnd : public ALUOperation {
protected:
  BitwiseAnd(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return a & b;}
};

class BitwiseOr : public ALUOperation {
protected:
  BitwiseOr(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return a | b;}
};

class BitwiseNor : public ALUOperation {
protected:
  BitwiseNor(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return ~(a | b);}
};

class BitwiseXor : public ALUOperation {
protected:
  BitwiseXor(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return a ^ b;}
};


class ShiftLeftLogical : public ALUOperation {
protected:
  ShiftLeftLogical(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return a << b;}
};

class ShiftRightLogical : public ALUOperation {
protected:
  ShiftRightLogical(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {
    return (uint32_t)a >> b;
  }
};

class ShiftRightArithmetic : public ALUOperation {
protected:
  ShiftRightArithmetic(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return a >> b;}
};


class SetEq : public ALUOperation {
protected:
  SetEq(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return a == b;}
};

class SetNE : public ALUOperation {
protected:
  SetNE(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return a != b;}
};

class SetLT : public ALUOperation {
protected:
  SetLT(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return a < b;}
};

class SetLTU : public ALUOperation {
protected:
  SetLTU(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {
    return ((uint32_t)a < (uint32_t)b);
  }
};

class SetGTE : public ALUOperation {
protected:
  SetGTE(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {return a >= b;}
};

class SetGTEU : public ALUOperation {
protected:
  SetGTEU(Instruction encoded) : ALUOperation(encoded) {}
  int32_t compute(int32_t a, int32_t b=0, bool p=false) const {
    return ((uint32_t)a >= (uint32_t)b);
  }
};


#endif /* SRC_ISA_OPERATIONS_ALUOPERATIONS_H_ */
