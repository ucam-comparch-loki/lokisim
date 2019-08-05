/*
 * InstructionFormats.h
 *
 * Slight specialisations over InstructionBase for each of the instruction
 * encoding formats. We now know how many operands there are, etc., so can
 * extract them, and we know which operands to request from the Core.
 *
 *  Created on: 2 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_INSTRUCTIONFORMATS_H_
#define SRC_ISA_INSTRUCTIONFORMATS_H_

#include "../InstructionBase.h"

template<class T>
class InstructionFetchFormat : public T {
protected:
  // | p |   opcode    |                 immediate                   |
  //   2        7                            23
  InstructionFetchFormat(Instruction encoded) :
      T(encoded),
      immediate(signExtend(encoded.getBits(0, 22), 23)) {
    // Nothing
  }

  const int32_t immediate;
};

template<class T>
class InstructionPredicatedFetchFormat : public T {
protected:
  // | p |   opcode    |             imm2             |     imm1     |
  //   2        7                     16                      7
  InstructionPredicatedFetchFormat(Instruction encoded) :
      T(encoded),
      immediate1(signExtend(encoded.getBits(0, 6), 7)),
      immediate2(signExtend(encoded.getBits(7, 22), 16)) {
    // Nothing
  }

  const int32_t immediate1;
  const int32_t immediate2;
};

template<class T>
class InstructionZeroRegisterNoChannelFormat : public T {
protected:
  // | p |   opcode    |    xxxxxxxxx    |         immediate         |
  //   2        7               9                      14
  InstructionZeroRegisterNoChannelFormat(Instruction encoded) :
      T(encoded),
      immediate(signExtend(encoded.getBits(0, 13), 14)) {
    // Nothing
  }

  const int32_t immediate;
};

template<class T>
class InstructionZeroRegisterFormat : public T {
protected:
  // | p |   opcode    |  xxxxx  |channel|         immediate         |
  //   2        7           5        4                 14
  InstructionZeroRegisterFormat(Instruction encoded) :
      T(encoded),
      immediate(signExtend(encoded.getBits(0, 13), 14)),
      outChannel(encoded.getBits(14, 17)) {
    // Nothing
  }

  const int32_t immediate;
  const ChannelIndex outChannel;
};

template<class T>
class InstructionOneRegisterNoChannelFormat : public T {
protected:
  // | p |   opcode    |  reg 1  | x |           immediate           |
  //   2        7           5      2                 16
  InstructionOneRegisterNoChannelFormat(Instruction encoded) :
      T(encoded),
      reg1(encoded.getBits(18, 22)),
      immediate(signExtend(encoded.getBits(0, 15), 16)) {
    // Nothing
  }

  const RegisterIndex reg1;
  const int32_t immediate;
};

template<class T>
class InstructionOneRegisterFormat : public T {
protected:
  // | p |   opcode    |  reg 1  |channel|         immediate         |
  //   2        7           5        4                 14
  InstructionOneRegisterFormat(Instruction encoded) :
      T(encoded),
      reg1(encoded.getBits(18, 22)),
      immediate(signExtend(encoded.getBits(0, 13), 14)),
      outChannel(encoded.getBits(14, 17)) {
    // Nothing
  }

  const RegisterIndex reg1;
  const int32_t immediate;
  const ChannelIndex outChannel;
};

template<class T>
class InstructionTwoRegisterNoChannelFormat : public T {
protected:
  // | p |   opcode    |  reg 1  | xxxx  |  reg 2  |    immediate    |
  //   2        7           5        4        5             9
  InstructionTwoRegisterNoChannelFormat(Instruction encoded) :
      T(encoded),
      reg1(encoded.getBits(18, 22)),
      reg2(encoded.getBits(9, 13)),
      immediate(signExtend(encoded.getBits(0, 8), 9)) {
    // Nothing
  }

  const RegisterIndex reg1;
  const RegisterIndex reg2;
  const int32_t immediate;
};

template<class T>
class InstructionTwoRegisterFormat : public T {
protected:
  // | p |   opcode    |  reg 1  |channel|  reg 2  |    immediate    |
  //   2        7           5        4        5             9
  InstructionTwoRegisterFormat(Instruction encoded) :
      T(encoded),
      reg1(encoded.getBits(18, 22)),
      reg2(encoded.getBits(9, 13)),
      immediate(signExtend(encoded.getBits(0, 8), 9)),
      outChannel(encoded.getBits(14, 17)) {
    // Nothing
  }

  const RegisterIndex reg1;
  const RegisterIndex reg2;
  const int32_t immediate;
  const ChannelIndex outChannel;
};

template<class T>
class InstructionTwoRegisterShiftFormat : public T {
protected:
  // | p |   opcode    |  reg 1  |channel|  reg 2  | xxxx  |immediate|
  //   2        7           5        4        5        4        5
  InstructionTwoRegisterShiftFormat(Instruction encoded) :
      T(encoded),
      reg1(encoded.getBits(18, 22)),
      reg2(encoded.getBits(9, 13)),
      immediate(signExtend(encoded.getBits(0, 4), 5)),
      outChannel(encoded.getBits(14, 17)) {
    // Nothing
  }

  const RegisterIndex reg1;
  const RegisterIndex reg2;
  const int32_t immediate;
  const ChannelIndex outChannel;
};

template<class T>
class InstructionThreeRegisterFormat : public T {
protected:
  // | p |   opcode    |  reg 1  |channel|  reg 2  |  reg 3  |  fn   |
  //   2        7           5        4        5         5        4
  InstructionThreeRegisterFormat(Instruction encoded) :
      T(encoded),
      reg1(encoded.getBits(18, 22)),
      reg2(encoded.getBits(9, 13)),
      reg3(encoded.getBits(4, 8)),
      function(static_cast<function_t>(encoded.getBits(0, 3))),
      outChannel(encoded.getBits(14, 17)) {
    // Nothing
  }

  const RegisterIndex reg1;
  const RegisterIndex reg2;
  const RegisterIndex reg3;
  const function_t function;
  const ChannelIndex outChannel;
};



#endif /* SRC_ISA_INSTRUCTIONFORMATS_H_ */
