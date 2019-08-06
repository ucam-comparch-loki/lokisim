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

template<class T>
class NoDest1Src : public T {
protected:

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

  int32_t operand1;
};

template<class T>
class Dest1Src : public T {
protected:

  Dest1Src(Instruction encoded) : T(encoded), destinationRegister(this->reg1) {
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

  const ChannelIndex destinationRegister;
  int32_t operand1;
  int32_t result;
};

template<class T>
class NoDest2Src : public T {
protected:

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

  int32_t operand1, operand2;
  int operandsReceived;
};

template<class T>
class Dest2Src : public T {
protected:

  Dest2Src(Instruction encoded) : T(encoded), destinationRegister(this->reg1) {
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

  const ChannelIndex destinationRegister;
  int32_t operand1, operand2;
  int operandsReceived;
  int32_t result;
};

template<class T>
class NoDest1Imm : public T {
protected:

  NoDest1Imm(Instruction encoded) : T(encoded), operand1(this->immediate) {
    // Nothing
  }

  const int32_t operand1;
};

template<class T>
class Dest1Imm : public T {
protected:

  Dest1Imm(Instruction encoded) :
      T(encoded),
      destinationRegister(this->reg1),
      operand1(this->immediate) {
    result = 0;
  }

  const ChannelIndex destinationRegister;
  const int32_t operand1;
  int32_t result;
};

template<class T>
class NoDest1Src1Imm : public T {
protected:

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

  int32_t operand1;
  const int32_t operand2;
};

template<class T>
class Dest1Src1Imm : public T {
protected:

  Dest1Src1Imm(Instruction encoded) :
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

  const ChannelIndex destinationRegister;
  int32_t operand1;
  const int32_t operand2;
  int32_t result;
};

template<class T>
class NoDest2Src1Imm : public T {
protected:

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

  int32_t operand1, operand2;
  const int32_t operand3;
  int operandsReceived;
};

template<class T>
class NoDest2Imm : public T {
protected:

  NoDest2Imm(Instruction encoded) :
      T(encoded),
      operand1(this->immediate1),
      operand2(this->immediate2) {
    // Nothing
  }

  const int32_t operand1, operand2;
};


#endif /* SRC_ISA_MIXINS_OPERANDSOURCE_H_ */
