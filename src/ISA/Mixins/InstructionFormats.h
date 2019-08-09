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

#include "../../Datatype/Instruction.h"

template<class T>
class FetchFormat : public T {
protected:
  // | p |   opcode    |                 immediate                   |
  //   2        7                            23
  FetchFormat(Instruction encoded) :
      T(encoded),
      immediate(this->signExtend(encoded.getBits(0, 22), 23)) {
    // Nothing
  }

  const int32_t immediate;
};

template<class T>
class PredicatedFetchFormat : public T {
protected:
  // | p |   opcode    |             imm2             |     imm1     |
  //   2        7                     16                      7
  PredicatedFetchFormat(Instruction encoded) :
      T(encoded),
      immediate1(this->signExtend(encoded.getBits(0, 6), 7)),
      immediate2(this->signExtend(encoded.getBits(7, 22), 16)) {
    // Nothing
  }

  const int32_t immediate1;
  const int32_t immediate2;
};

template<class T>
class ZeroRegNoChannelFormat : public T {
protected:
  // | p |   opcode    |    xxxxxxxxx    |         immediate         |
  //   2        7               9                      14
  ZeroRegNoChannelFormat(Instruction encoded) :
      T(encoded),
      immediate(this->signExtend(encoded.getBits(0, 13), 14)) {
    // Nothing
  }

  const int32_t immediate;
};

template<class T>
class ZeroRegFormat : public T {
protected:
  // | p |   opcode    |  xxxxx  |channel|         immediate         |
  //   2        7           5        4                 14
  ZeroRegFormat(Instruction encoded) :
      T(encoded),
      immediate(this->signExtend(encoded.getBits(0, 13), 14)),
      outChannel(encoded.getBits(14, 17)) {
    // Nothing
  }

  const int32_t immediate;
  const ChannelIndex outChannel;
};

template<class T>
class OneRegNoChannelFormat : public T {
protected:
  // | p |   opcode    |  reg 1  | x |           immediate           |
  //   2        7           5      2                 16
  OneRegNoChannelFormat(Instruction encoded) :
      T(encoded),
      reg1(encoded.getBits(18, 22)),
      immediate(this->signExtend(encoded.getBits(0, 15), 16)) {
    // Nothing
  }

  const RegisterIndex reg1;
  const int32_t immediate;
};

template<class T>
class OneRegFormat : public T {
protected:
  // | p |   opcode    |  reg 1  |channel|         immediate         |
  //   2        7           5        4                 14
  OneRegFormat(Instruction encoded) :
      T(encoded),
      reg1(encoded.getBits(18, 22)),
      immediate(this->signExtend(encoded.getBits(0, 13), 14)),
      outChannel(encoded.getBits(14, 17)) {
    // Nothing
  }

  const RegisterIndex reg1;
  const int32_t immediate;
  const ChannelIndex outChannel;
};

template<class T>
class TwoRegNoChannelFormat : public T {
protected:
  // | p |   opcode    |  reg 1  | xxxx  |  reg 2  |    immediate    |
  //   2        7           5        4        5             9
  TwoRegNoChannelFormat(Instruction encoded) :
      T(encoded),
      reg1(encoded.getBits(18, 22)),
      reg2(encoded.getBits(9, 13)),
      immediate(this->signExtend(encoded.getBits(0, 8), 9)) {
    // Nothing
  }

  const RegisterIndex reg1;
  const RegisterIndex reg2;
  const int32_t immediate;
};

template<class T>
class TwoRegFormat : public T {
protected:
  // | p |   opcode    |  reg 1  |channel|  reg 2  |    immediate    |
  //   2        7           5        4        5             9
  TwoRegFormat(Instruction encoded) :
      T(encoded),
      reg1(encoded.getBits(18, 22)),
      reg2(encoded.getBits(9, 13)),
      immediate(this->signExtend(encoded.getBits(0, 8), 9)),
      outChannel(encoded.getBits(14, 17)) {
    // Nothing
  }

  const RegisterIndex reg1;
  const RegisterIndex reg2;
  const int32_t immediate;
  const ChannelIndex outChannel;
};

template<class T>
class TwoRegShiftFormat : public T {
protected:
  // | p |   opcode    |  reg 1  |channel|  reg 2  | xxxx  |immediate|
  //   2        7           5        4        5        4        5
  TwoRegShiftFormat(Instruction encoded) :
      T(encoded),
      reg1(encoded.getBits(18, 22)),
      reg2(encoded.getBits(9, 13)),
      immediate(this->signExtend(encoded.getBits(0, 4), 5)),
      outChannel(encoded.getBits(14, 17)) {
    // Nothing
  }

  const RegisterIndex reg1;
  const RegisterIndex reg2;
  const int32_t immediate;
  const ChannelIndex outChannel;
};

template<class T>
class ThreeRegFormat : public T {
protected:
  // | p |   opcode    |  reg 1  |channel|  reg 2  |  reg 3  |  fn   |
  //   2        7           5        4        5         5        4
  ThreeRegFormat(Instruction encoded) :
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
