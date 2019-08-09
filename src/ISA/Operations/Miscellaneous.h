/*
 * Miscellaneous.h
 *
 *  Created on: 8 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_OPERATIONS_MISCELLANEOUS_H_
#define SRC_ISA_OPERATIONS_MISCELLANEOUS_H_

#include "../../Datatype/Instruction.h"

// Do no computation: copy the first operand into the result.
template<class T>
class NoOp : public T {
public:
  NoOp(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

// Load lower immediate. Put a 16 bit value into a register.
template<class T>
class LoadLowerImmediate : public T {
public:
  LoadLowerImmediate(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 & 0xFFFF;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

// Load upper immediate. Put a 16 bit value into the upper half of a register.
template<class T>
class LoadUpperImmediate : public T {
public:
  LoadUpperImmediate(Instruction encoded) : T(encoded) {}
  void compute() {
    this->result = this->operand1 | (this->operand2 << 16);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

// Get channel map table entry.
template<class T>
class GetChannelMap : public T {
public:
  GetChannelMap(Instruction encoded) : T(encoded) {}
  void readCMTCallback(EncodedCMTEntry value) {
    T::readCMTCallback(value);

    // Also store the read value as this instruction's result.
    this->result = value;

    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

// Indirect register read. The content of the given register determines which
// register to access.
template<class T>
class IndirectRead : public T {
public:
  IndirectRead(Instruction encoded) : T(encoded) {}

  void readRegisters() {
    this->core->readRegister(this->reg2, REGISTER_PORT_1);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    // Slight hack: use port 1 for the original request, and port 2 for the
    // indirect request.
    switch (port) {
      case REGISTER_PORT_1:
        this->core->readRegister(value, REGISTER_PORT_2);
        break;
      case REGISTER_PORT_2:
        this->result = value;
        this->finished.notify(sc_core::SC_ZERO_TIME);
        break;
    }
  }
};

// Indirect register write.
template<class T>
class IndirectWrite : public T {
public:
  IndirectWrite(Instruction encoded) : T(encoded) {
    destinationRegister = result = operandsReceived = 0;
  }

  void readRegisters() {
    this->core->readRegister(this->reg1, REGISTER_PORT_1);
    this->core->readRegister(this->reg2, REGISTER_PORT_2);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    switch (port) {
      case REGISTER_PORT_1:
        destinationRegister = value; break;
      case REGISTER_PORT_2:
        result = value; break;
    }
    operandsReceived++;

    if (operandsReceived == 2)
      this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  RegisterIndex destinationRegister;
  int32_t result;
  int operandsReceived;
};

// System call.
template<class T>
class SystemCall : public T {
public:
  SystemCall(Instruction encoded) : T(encoded) {}

  void compute() {
    this->core->syscall(this->operand1);
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};

// Remote execute.
template<class T>
class RemoteExecute : public T {
public:
  RemoteExecute(Instruction encoded) : T(encoded) {}

  void compute() {
    ChannelID address = this->core->getNetworkDestination(this->channelMapping);
    this->core->startRemoteExecution(address);
    this->finished.notify(sc_core::SC_ZERO_TIME);
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
class RemoteNextIPK : public T {
public:
  RemoteNextIPK(Instruction encoded) : T(encoded) {result = 0;}

  void compute() {
    result = Instruction("nxipk").toInt();
  }

protected:
  int32_t result;
};


#endif /* SRC_ISA_OPERATIONS_MISCELLANEOUS_H_ */
