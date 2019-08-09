/*
 * OperandSource.h
 *
 * There are many different possibilities for gathering operands. Instructions
 * may encode 0 to 3 registers and 0 to 2 immediates. Sometimes the first
 * register is the destination, so should not be read.
 *
 * The following options are supported:
 *  * No destination, 1 register,  0 immediates
 *  * 1 destination,  1 register,  0 immediates
 *  * No destination, 2 registers, 0 immediates
 *  * 1 destination,  2 registers, 0 immediates
 *  * No destination, 0 registers, 1 immediate
 *  * 1 destination,  0 registers, 1 immediate
 *  * No destination, 1 register,  1 immediate
 *  * 1 destination,  1 register,  1 immediate
 *  * No destination, 2 registers, 1 immediate
 *  * No destination, 0 registers, 2 immediates
 *
 * There is a strong correlation between these options and the available
 * instruction formats, but the correlation is not good enough to directly
 * subtype those mix-ins. e.g. 1 register vs 1 register no channel.
 *
 *  Created on: 5 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_MIXINS_OPERANDSOURCE_H_
#define SRC_ISA_MIXINS_OPERANDSOURCE_H_

#include "../../Datatype/Instruction.h"
#include "../CoreInterface.h"


// An instruction which writes to the register file.
// This mix-in must wrap an OperandSource which specifies a destination.
template<class T>
class WriteRegister : public T {
public:
  WriteRegister(Instruction encoded) : T(encoded) {}

  void writeRegisters() {
    this->core->writeRegister(this->destinationRegister, this->result);
  }

  void writeRegistersCallback() {
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }
};


// No destination register, one source register.
template<class T>
class NoDest1Src : public T {
public:

  NoDest1Src(Instruction encoded) : T(encoded) {
    operand1 = 0;
  }

  void readRegisters() {
    this->core->readRegister(this->reg1, REGISTER_PORT_1);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    assert(port == REGISTER_PORT_1);
    operand1 = value;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  int32_t operand1;
};

// Destination register and one source register.
template<class T>
class Dest1SrcInternal : public T {
public:

  Dest1SrcInternal(Instruction encoded) :
      T(encoded),
      destinationRegister(this->reg1) {
    operand1 = 0;
    result = 0;
  }

  void readRegisters() {
    this->core->readRegister(this->reg2, REGISTER_PORT_1);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    assert(port == REGISTER_PORT_1);
    operand1 = value;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  const RegisterIndex destinationRegister;
  int32_t operand1;
  int32_t result;
};
template<class T>
using Dest1Src = WriteRegister<Dest1SrcInternal<T>>;


// One register, used as both source and destination.
template<class T>
class Dest1SrcSharedInternal : public T {
public:

  Dest1SrcSharedInternal(Instruction encoded) :
      T(encoded),
      destinationRegister(this->reg1) {
    operand1 = 0;
    result = 0;
  }

  void readRegisters() {
    this->core->readRegister(this->reg1, REGISTER_PORT_1);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    assert(port == REGISTER_PORT_1);
    operand1 = value;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  const RegisterIndex destinationRegister;
  int32_t operand1;
  int32_t result;
};
template<class T>
using Dest1SrcShared = WriteRegister<Dest1SrcSharedInternal<T>>;


// No destination register, two source registers.
template<class T>
class NoDest2Src : public T {
public:

  NoDest2Src(Instruction encoded) : T(encoded) {
    operand1 = operand2 = operandsReceived = 0;
  }

  void readRegisters() {
    this->core->readRegister(this->reg1, REGISTER_PORT_1);
    this->core->readRegister(this->reg2, REGISTER_PORT_2);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    switch (port) {
      case REGISTER_PORT_1:
        operand1 = value; break;
      case REGISTER_PORT_2:
        operand2 = value; break;
    }
    operandsReceived++;

    if (operandsReceived == 2)
      this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  int32_t operand1, operand2;
  int operandsReceived;
};


// Destination register and two source registers.
template<class T>
class Dest2SrcInternal : public T {
public:

  Dest2SrcInternal(Instruction encoded) :
      T(encoded),
      destinationRegister(this->reg1) {
    operand1 = operand2 = operandsReceived = result = 0;
  }

  void readRegisters() {
    this->core->readRegister(this->reg2, REGISTER_PORT_1);
    this->core->readRegister(this->reg3, REGISTER_PORT_2);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    switch (port) {
      case REGISTER_PORT_1:
        operand1 = value; break;
      case REGISTER_PORT_2:
        operand2 = value; break;
    }
    operandsReceived++;

    if (operandsReceived == 2)
      this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  const RegisterIndex destinationRegister;
  int32_t operand1, operand2;
  int operandsReceived;
  int32_t result;
};
template<class T>
using Dest2Src = WriteRegister<Dest2SrcInternal<T>>;


// No destination register, one immediate.
template<class T>
class NoDest1Imm : public T {
public:

  NoDest1Imm(Instruction encoded) : T(encoded), operand1(this->immediate) {
    // Nothing
  }

protected:
  const int32_t operand1;
};

// Destination register and one immediate.
template<class T>
class Dest1Imm : public T {
public:

  Dest1Imm(Instruction encoded) :
      T(encoded),
      destinationRegister(this->reg1),
      operand1(this->immediate) {
    result = 0;
  }

protected:
  const RegisterIndex destinationRegister;
  const int32_t operand1;
  int32_t result;
};


// No destination register, one source register, one immediate.
template<class T>
class NoDest1Src1Imm : public T {
public:

  NoDest1Src1Imm(Instruction encoded) : T(encoded), operand2(this->immediate) {
    operand1 = 0;
  }

  void readRegisters() {
    this->core->readRegister(this->reg1, REGISTER_PORT_1);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    assert(port == REGISTER_PORT_1);
    operand1 = value;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  int32_t operand1;
  const int32_t operand2;
};


// Destination register, one source register, one immediate.
template<class T>
class Dest1Src1ImmInternal : public T {
public:

  Dest1Src1ImmInternal(Instruction encoded) :
      T(encoded),
      destinationRegister(this->reg1),
      operand2(this->immediate) {
    operand1 = result = 0;
  }

  void readRegisters() {
    this->core->readRegister(this->reg2, REGISTER_PORT_1);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    assert(port == REGISTER_PORT_1);
    operand1 = value;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  const RegisterIndex destinationRegister;
  int32_t operand1;
  const int32_t operand2;
  int32_t result;
};
template<class T>
using Dest1Src1Imm = WriteRegister<Dest1Src1ImmInternal<T>>;


// One register used as both source and destination, and one immediate.
template<class T>
class Dest1SrcShared1ImmInternal : public T {
public:

  Dest1SrcShared1ImmInternal(Instruction encoded) :
      T(encoded),
      destinationRegister(this->reg1),
      operand2(this->immediate) {
    operand1 = result = 0;
  }

  void readRegisters() {
    this->core->readRegister(this->reg1, REGISTER_PORT_1);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    assert(port == REGISTER_PORT_1);
    operand1 = value;
    this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  const RegisterIndex destinationRegister;
  int32_t operand1;
  const int32_t operand2;
  int32_t result;
};
template<class T>
using Dest1SrcShared1Imm = WriteRegister<Dest1SrcShared1ImmInternal<T>>;


// No destination register, two source registers, one immediate.
template<class T>
class NoDest2Src1Imm : public T {
public:

  NoDest2Src1Imm(Instruction encoded) : T(encoded), operand3(this->immediate) {
    operand1 = operand2 = operandsReceived = 0;
  }

  void readRegisters() {
    this->core->readRegister(this->reg1, REGISTER_PORT_1);
    this->core->readRegister(this->reg2, REGISTER_PORT_2);
  }

  void readRegistersCallback(RegisterPort port, int32_t value) {
    switch (port) {
      case REGISTER_PORT_1:
        operand1 = value; break;
      case REGISTER_PORT_2:
        operand2 = value; break;
    }
    operandsReceived++;

    if (operandsReceived == 2)
      this->finished.notify(sc_core::SC_ZERO_TIME);
  }

protected:
  int32_t operand1, operand2;
  const int32_t operand3;
  int operandsReceived;
};


// No destination register, two immediates.
template<class T>
class NoDest2Imm : public T {
public:

  NoDest2Imm(Instruction encoded) :
      T(encoded),
      operand1(this->immediate1),
      operand2(this->immediate2) {
    // Nothing
  }

protected:
  const int32_t operand1, operand2;
};


#endif /* SRC_ISA_MIXINS_OPERANDSOURCE_H_ */
