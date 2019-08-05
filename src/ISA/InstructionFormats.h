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

#include "InstructionBase.h"

class InstructionFetchFormat : public InstructionBase {
public:
  InstructionFetchFormat(Instruction encoded);

protected:
  const int32_t immediate;
};

class InstructionPredicatedFetchFormat : public InstructionBase {
public:
  InstructionPredicatedFetchFormat(Instruction encoded);

protected:
  const int32_t immediate1;
  const int32_t immediate2;
};

class InstructionZeroRegisterNoChannelFormat : public InstructionBase {
public:
  InstructionZeroRegisterNoChannelFormat(Instruction encoded);

protected:
  const int32_t immediate;
};

class InstructionZeroRegisterFormat : public InstructionZeroRegisterNoChannelFormat {
public:
  InstructionZeroRegisterFormat(Instruction encoded);

  virtual void readCMT();
  virtual void readCMTCallback(EncodedCMTEntry value);

protected:
  const ChannelIndex outChannel;
  EncodedCMTEntry outChannelMapping;
};

class InstructionOneRegisterNoChannelFormat : public InstructionBase {
public:
  InstructionOneRegisterNoChannelFormat(Instruction encoded);

protected:
  const RegisterIndex reg1;
  const int32_t immediate;
};

class InstructionOneRegisterFormat : public InstructionZeroRegisterFormat {
public:
  InstructionOneRegisterFormat(Instruction encoded);

protected:
  const RegisterIndex reg1;
};

class InstructionTwoRegisterNoChannelFormat : public InstructionBase {
public:
  InstructionTwoRegisterNoChannelFormat(Instruction encoded);

protected:
  const RegisterIndex reg1;
  const RegisterIndex reg2;
  const int32_t immediate;
};

class InstructionTwoRegisterFormat : public InstructionTwoRegisterNoChannelFormat {
public:
  InstructionTwoRegisterFormat(Instruction encoded);

  virtual void readCMT();
  virtual void readCMTCallback(EncodedCMTEntry value);

protected:
  const ChannelIndex outChannel;
  EncodedCMTEntry outChannelMapping;
};

class InstructionTwoRegisterShiftFormat : public InstructionTwoRegisterFormat {
public:
  InstructionTwoRegisterShiftFormat(Instruction encoded);
};

class InstructionThreeRegisterFormat : public InstructionBase {
public:
  InstructionThreeRegisterFormat(Instruction encoded);

  virtual void readCMT();
  virtual void readCMTCallback(EncodedCMTEntry value);

protected:
  const RegisterIndex reg1;
  const RegisterIndex reg2;
  const RegisterIndex reg3;
  const function_t function;
  const ChannelIndex outChannel;
  EncodedCMTEntry outChannelMapping;
};



#endif /* SRC_ISA_INSTRUCTIONFORMATS_H_ */
