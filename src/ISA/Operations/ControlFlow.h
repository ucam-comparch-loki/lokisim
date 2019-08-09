/*
 * ControlFlow.h
 *
 *  Created on: 8 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_OPERATIONS_CONTROLFLOW_H_
#define SRC_ISA_OPERATIONS_CONTROLFLOW_H_

#include "../../Datatype/Instruction.h"

// Fetch: request an instruction packet from the memory address generated by
// `compute()`. This mix-in must wrap `Relative`, and any computation operation.
template<class T>
class InstructionFetch : public T {
public:
  InstructionFetch(Instruction encoded) : T(encoded), fetchChannel(-1) {}

  void readCMT() {
    this->core->readCMT(0);
  }

  void readCMTCallback(EncodedCMTEntry value) {
    fetchChannel = ChannelMapEntry::MemoryChannel(value);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

  void compute() {
    T::compute();

    this->core->fetch(this->result, fetchChannel, true, false);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  ChannelMapEntry::MemoryChannel fetchChannel;
};

// Compute an address relative to the address of the current instruction packet.
// The operation wrapped must not already read registers.
template<class T>
class Relative : public T {
public:
  Relative(Instruction encoded) : T(encoded) {currentIPK = -1;}

  void readRegisters() {
    // Hard-coded use of port 1 is why the base instruction can't read
    // registers. Add another "fake" port to address this.
    this->core->readRegister(1, REGISTER_PORT_1);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    assert(port == REGISTER_PORT_1);
    currentIPK = value;
  }

  void compute() {
    T::compute();

    this->result += currentIPK;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  MemoryAddr currentIPK;
};

// Continually execute the same instruction packet until interrupted.
template<class T>
class PersistentFetch : public InstructionFetch<T> {
public:
  PersistentFetch(Instruction encoded) : InstructionFetch<T>(encoded) {}

  void compute() {
    T::compute();

    this->core->fetch(this->result, this->fetchChannel, true, true);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

// Request instructions from memory, but do not execute them. This is useful
// for ensuring instructions are in particular locations relative to each other.
template<class T>
class NoJumpFetch : public InstructionFetch<T> {
public:
  NoJumpFetch(Instruction encoded) : InstructionFetch<T>(encoded) {}

  void compute() {
    T::compute();

    this->core->fetch(this->result, this->fetchChannel, false, false);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

// Jump to a relative *position* in the current instruction source, rather than
// specifying an address. This can be faster than a Fetch because cache tags do
// not need to be checked.
template<class T>
class InBufferJump : public T {
public:
  InBufferJump(Instruction encoded) : T(encoded) {}

  void compute() {
    this->core->jump(this->operand1);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};


#endif /* SRC_ISA_OPERATIONS_CONTROLFLOW_H_ */
