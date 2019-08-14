/*
 * Miscellaneous.h
 *
 *  Created on: 8 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_OPERATIONS_MISCELLANEOUS_H_
#define SRC_ISA_OPERATIONS_MISCELLANEOUS_H_

#include "../../Datatype/Instruction.h"

namespace ISA {

// Do no computation: copy the first operand into the result.
template<class T>
class NoOp : public Has1Operand<HasResult<T>> {
public:
  NoOp(Instruction encoded) : Has1Operand<HasResult<T>>(encoded) {}
  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    this->result = this->operand1;
    this->core->computeLatency(this->opcode);
  }
  void computeCallback(int32_t unused) {
    this->finishedPhase(this->INST_COMPUTE);
  }
};

// Load lower immediate. Put a 16 bit value into a register.
template<class T>
class LoadLowerImmediate : public Has1Operand<HasResult<T>> {
public:
  LoadLowerImmediate(Instruction encoded) : Has1Operand<HasResult<T>>(encoded) {
    // Nothing
  }
  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    this->result = this->operand1 & 0xFFFF;
    this->core->computeLatency(this->opcode);
  }
  void computeCallback(int32_t unused) {
    this->finishedPhase(this->INST_COMPUTE);
  }
};

// Load upper immediate. Put a 16 bit value into the upper half of a register.
template<class T>
class LoadUpperImmediate : public Has2Operands<HasResult<T>> {
public:
  LoadUpperImmediate(Instruction encoded) :
      Has2Operands<HasResult<T>>(encoded) {
    // Nothing
  }
  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    this->result = this->operand1 | (this->operand2 << 16);
    this->core->computeLatency(this->opcode);
  }
  void computeCallback(int32_t unused) {
    this->finishedPhase(this->INST_COMPUTE);
  }
};

// Get channel map table entry.
template<class T>
class GetChannelMap : public Has1Operand<HasResult<T>> {
public:
  GetChannelMap(Instruction encoded) : Has1Operand<HasResult<T>>(encoded) {}
  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    this->core->readCMT(this->operand1);
  }
  void computeCallback(int32_t value) {
    this->result = value;
    this->finishedPhase(this->INST_COMPUTE);
  }
  void readCMTCallback(EncodedCMTEntry value) {
    // This instruction can potentially read the channel map table in multiple
    // phases of execution.
    if (this->phaseInProgress(this->INST_CMT_READ))
      T::readCMTCallback(value);
    else
      computeCallback(value);
  }
};

// Indirect register read. The content of the given register determines which
// register to access.
template<class T>
class IndirectRead : public Has1Operand<HasResult<T>> {
public:
  IndirectRead(Instruction encoded) : Has1Operand<HasResult<T>>(encoded) {}

  void readRegisters() {
    this->startingPhase(this->INST_REG_READ);
    this->core->readRegister(this->reg2, REGISTER_PORT_1);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    // Slight hack: use port 1 for the original request, and port 2 for the
    // indirect request. In practice, any registers capable of indirecting
    // would probably have a special fast interface.
    switch (port) {
      case REGISTER_PORT_1:
        this->core->readRegister(value, REGISTER_PORT_2);
        break;
      case REGISTER_PORT_2:
        this->result = value;
        this->finishedPhase(this->INST_REG_READ);
        break;
    }
  }
};

// Indirect register write.
template<class T>
class IndirectWrite : public Has1Operand<HasResult<T>> {
public:
  IndirectWrite(Instruction encoded) : Has1Operand<HasResult<T>>(encoded) {
    destinationRegister = operandsReceived = 0;
  }

  void readRegisters() {
    this->startingPhase(this->INST_REG_READ);
    this->core->readRegister(this->reg1, REGISTER_PORT_1);
    this->core->readRegister(this->reg2, REGISTER_PORT_2);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    switch (port) {
      case REGISTER_PORT_1:
        destinationRegister = value; break;
      case REGISTER_PORT_2:
        this->result = value; break;
    }
    operandsReceived++;

    if (operandsReceived == 2)
      this->finishedPhase(this->INST_REG_READ);
  }

protected:
  RegisterIndex destinationRegister;
  int operandsReceived;
};

// System call.
template<class T>
class SystemCall : public Has1Operand<T> {
public:
  SystemCall(Instruction encoded) : Has1Operand<T>(encoded) {}

  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    this->core->syscall(this->operand1);
  }
  void computeCallback(int32_t unused) {
    this->finishedPhase(this->INST_COMPUTE);
  }
};

// Remote execute.
template<class T>
class RemoteExecute : public T {
public:
  RemoteExecute(Instruction encoded) : T(encoded) {}

  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    ChannelID address = this->core->getNetworkDestination(this->channelMapping);
    this->core->startRemoteExecution(address);
    this->finishedPhase(this->INST_COMPUTE);
  }
};

// Next instruction packet. Not an instruction to be executed, but as soon as
// a core notices this instruction arrive, it immediately terminates the packet
// it is currently working on.
template<class T>
class NextIPK : public T {
public:
  NextIPK(Instruction encoded) : T(encoded) {
    assert(false && "Not implemented");
  }
};

// Remote next instruction packet. Send a `next instruction packet` command to
// a remote core.
template<class T>
class RemoteNextIPK : public HasResult<T> {
public:
  RemoteNextIPK(Instruction encoded) : HasResult<T>(encoded) {result = 0;}

  void compute() {
    this->startingPhase(this->INST_COMPUTE);
    result = Instruction("nxipk").toInt();
    this->finishedPhase(this->INST_COMPUTE);
  }

protected:
  int32_t result;
};

} // end namespace

#endif /* SRC_ISA_OPERATIONS_MISCELLANEOUS_H_ */
