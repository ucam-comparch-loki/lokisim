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

// Base class for all memory operations. Some extra fields and methods must be
// available to allow proper network communication.
template<class T>
class MemoryInstruction : public T {
protected:
  MemoryInstruction(Instruction encoded, uint totalFlits, MemoryOpcode op) :
      T(encoded),
      totalFlits(totalFlits),
      memoryOp(op) {
    // Nothing
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
};

// Operations which behave like loads. i.e. They produce one flit, which is the
// address to load from, and do no other work.
template<class T>
class LoadLike : public MemoryInstruction<T> {
protected:
  LoadLike(Instruction encoded, MemoryOpcode op) :
      MemoryInstruction(encoded, 1, op) {
    // Nothing
  }

  void compute() {
    this->result = this->operand1 + this->operand2;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

  int32_t getPayload(uint flitNumber) const {
    return this->result;
  }
};

// Operations which behave like stores. i.e. They produce two flits: the head
// contains an address and the body contains the data to be stored.
template<class T>
class StoreLike : public MemoryInstruction<T> {
protected:
  StoreLike(Instruction encoded, MemoryOpcode op) :
      MemoryInstruction(encoded, 2, op) {
    // Nothing
  }

  void compute() {
    this->result = this->operand2 + this->operand3;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

  int32_t getPayload(uint flitNumber) const {
    assert(flitNumber < 2);

    if (flitNumber == 0)
      return this->result;
    else
      return this->operand1;
  }
};


template<class T>
class LoadWord : public LoadLike<T> {
protected:
  LoadWord(Instruction encoded) : LoadLike<T>(encoded, LOAD_W) {}
};

template<class T>
class LoadHalfWord : public LoadLike<T> {
protected:
  LoadHalfWord(Instruction encoded) : LoadLike<T>(encoded, LOAD_HW) {}
};

template<class T>
class LoadByte : public LoadLike<T> {
protected:
  LoadByte(Instruction encoded) : LoadLike<T>(encoded, LOAD_B) {}
};

template<class T>
class LoadLinked : public LoadLike<T> {
protected:
  LoadLinked(Instruction encoded) : LoadLike<T>(encoded, LOAD_LINKED) {}
};


template<class T>
class StoreWord : public StoreLike<T> {
protected:
  StoreWord(Instruction encoded) : StoreLike<T>(encoded, STORE_W) {}
};

template<class T>
class StoreHalfWord : public StoreLike<T> {
protected:
  StoreHalfWord(Instruction encoded) : StoreLike<T>(encoded, STORE_HW) {}
};

template<class T>
class StoreByte : public StoreLike<T> {
protected:
  StoreByte(Instruction encoded) : StoreLike<T>(encoded, STORE_B) {}
};

template<class T>
class StoreConditional : public StoreLike<T> {
protected:
  StoreConditional(Instruction encoded) : StoreLike<T>(encoded, STORE_CONDITIONAL) {}
};

template<class T>
class LoadAndAdd : public StoreLike<T> {
protected:
  LoadAndAdd(Instruction encoded) : StoreLike<T>(encoded, LOAD_AND_ADD) {}
};

template<class T>
class LoadAndOr : public StoreLike<T> {
protected:
  LoadAndOr(Instruction encoded) : StoreLike<T>(encoded, LOAD_AND_OR) {}
};

template<class T>
class LoadAndAnd : public StoreLike<T> {
protected:
  LoadAndAnd(Instruction encoded) : StoreLike<T>(encoded, LOAD_AND_AND) {}
};

template<class T>
class LoadAndXor : public StoreLike<T> {
protected:
  LoadAndXor(Instruction encoded) : StoreLike<T>(encoded, LOAD_AND_XOR) {}
};

template<class T>
class Exchange : public StoreLike<T> {
protected:
  Exchange(Instruction encoded) : StoreLike<T>(encoded, EXCHANGE) {}
};


#endif /* SRC_ISA_OPERATIONS_MEMORY_H_ */
