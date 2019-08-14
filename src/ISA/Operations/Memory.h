/*
 * Memory.h
 *
 *  Created on: 6 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_OPERATIONS_MEMORY_H_
#define SRC_ISA_OPERATIONS_MEMORY_H_

#include "../../Datatype/Instruction.h"
#include "../../Memory/MemoryTypes.h"

namespace ISA {

// Base class for all memory operations. Some extra fields and methods must be
// available to allow proper network communication.
template<class T>
class MemoryInstruction : public T {
protected:
  MemoryInstruction(Instruction encoded, uint totalFlits, MemoryOpcode op) :
      T(encoded),
      totalFlits(totalFlits),
      memoryOp(op) {
    address = -1;
  }

  int32_t getPayload(uint flitNumber) const {
    assert(false && "Not implemented in base class");
    return -1;
  }

  MemoryOpcode getMemoryOp(uint flitNumber) const {
    if (flitNumber == 0)
      return memoryOp;
    else if (flitNumber < totalFlits - 1)
      return PAYLOAD;
    else
      return PAYLOAD_EOP;
  }

  // The total number of flits to be sent by this request.
  const uint totalFlits;

  // The opcode to be used by the head flit. All other flits are assumed to be
  // payloads.
  const MemoryOpcode memoryOp;

  // The address to access.
  MemoryAddr address;
};

// Operations which behave like loads. i.e. They produce one flit, which is the
// address to load from, and do no other work.
template<class T>
class LoadLike : public MemoryInstruction<Has2Operands<T>> {
protected:
  LoadLike(Instruction encoded, MemoryOpcode op) :
      MemoryInstruction<Has2Operands<T>>(encoded, 1, op) {
    // Nothing
  }

  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    this->address = this->operand1 + this->operand2;

    this->core->computeLatency(OP_ADDU);
  }

  void computeCallback(int32_t unused) {
    this->finishedPhase(this->INST_COMPUTE);
  }

  int32_t getPayload(uint flitNumber) const {
    return this->address;
  }
};

// Operations which behave like stores. i.e. They produce two flits: the head
// contains an address and the body contains the data to be stored.
template<class T>
class StoreLike : public MemoryInstruction<Has3Operands<T>> {
protected:
  StoreLike(Instruction encoded, MemoryOpcode op) :
      MemoryInstruction<Has3Operands<T>>(encoded, 2, op) {
    // Nothing
  }

  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    this->address = this->operand2 + this->operand3;

    this->core->computeLatency(OP_ADDU);
  }

  void computeCallback(int32_t unused) {
    this->finishedPhase(this->INST_COMPUTE);
  }

  int32_t getPayload(uint flitNumber) const {
    assert(flitNumber < 2);

    if (flitNumber == 0)
      return this->address;
    else
      return this->operand1;
  }
};


template<class T>
class LoadWord : public LoadLike<T> {
public:
  LoadWord(Instruction encoded) : LoadLike<T>(encoded, LOAD_W) {}
};

template<class T>
class LoadHalfWord : public LoadLike<T> {
public:
  LoadHalfWord(Instruction encoded) : LoadLike<T>(encoded, LOAD_HW) {}
};

template<class T>
class LoadByte : public LoadLike<T> {
public:
  LoadByte(Instruction encoded) : LoadLike<T>(encoded, LOAD_B) {}
};

template<class T>
class LoadLinked : public LoadLike<T> {
public:
  LoadLinked(Instruction encoded) : LoadLike<T>(encoded, LOAD_LINKED) {}
};


template<class T>
class StoreWord : public StoreLike<T> {
public:
  StoreWord(Instruction encoded) : StoreLike<T>(encoded, STORE_W) {}
};

template<class T>
class StoreHalfWord : public StoreLike<T> {
public:
  StoreHalfWord(Instruction encoded) : StoreLike<T>(encoded, STORE_HW) {}
};

template<class T>
class StoreByte : public StoreLike<T> {
public:
  StoreByte(Instruction encoded) : StoreLike<T>(encoded, STORE_B) {}
};

template<class T>
class StoreConditional : public StoreLike<T> {
public:
  StoreConditional(Instruction encoded) : StoreLike<T>(encoded, STORE_CONDITIONAL) {}
};

template<class T>
class LoadAndAdd : public StoreLike<T> {
public:
  LoadAndAdd(Instruction encoded) : StoreLike<T>(encoded, LOAD_AND_ADD) {}
};

template<class T>
class LoadAndOr : public StoreLike<T> {
public:
  LoadAndOr(Instruction encoded) : StoreLike<T>(encoded, LOAD_AND_OR) {}
};

template<class T>
class LoadAndAnd : public StoreLike<T> {
public:
  LoadAndAnd(Instruction encoded) : StoreLike<T>(encoded, LOAD_AND_AND) {}
};

template<class T>
class LoadAndXor : public StoreLike<T> {
public:
  LoadAndXor(Instruction encoded) : StoreLike<T>(encoded, LOAD_AND_XOR) {}
};

template<class T>
class Exchange : public StoreLike<T> {
public:
  Exchange(Instruction encoded) : StoreLike<T>(encoded, EXCHANGE) {}
};

} // end namespace


#endif /* SRC_ISA_OPERATIONS_MEMORY_H_ */
