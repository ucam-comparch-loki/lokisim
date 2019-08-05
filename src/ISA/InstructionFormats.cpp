/*
 * InstructionFormats.cpp
 *
 * Formats can be found in /usr/groups/comparch-loki/isa/formats.isa.
 *
 *  Created on: 2 Aug 2019
 *      Author: db434
 */

#include "InstructionFormats.h"
#include "systemc"

// | p |   opcode    |                 immediate                   |
//   2        7                            23
InstructionFetchFormat::InstructionFetchFormat(Instruction encoded) :
    InstructionBase(encoded),
    immediate(signExtend(encoded.getBits(0, 22), 23)) {
  // Nothing
}

// | p |   opcode    |             imm2             |     imm1     |
//   2        7                     16                      7
InstructionPredicatedFetchFormat::InstructionPredicatedFetchFormat(Instruction encoded) :
    InstructionBase(encoded),
    immediate1(signExtend(encoded.getBits(0, 6), 7)),
    immediate2(signExtend(encoded.getBits(7, 22), 16)) {
  // Nothing
}

// | p |   opcode    |    xxxxxxxxx    |         immediate         |
//   2        7               9                      14
InstructionZeroRegisterNoChannelFormat::InstructionZeroRegisterNoChannelFormat(Instruction encoded) :
    InstructionBase(encoded),
    immediate(signExtend(encoded.getBits(0, 13), 14)) {
  // Nothing
}

// | p |   opcode    |  xxxxx  |channel|         immediate         |
//   2        7           5        4                 14
InstructionZeroRegisterFormat::InstructionZeroRegisterFormat(Instruction encoded) :
    InstructionZeroRegisterNoChannelFormat(encoded),
    outChannel(encoded.getBits(14, 17)) {
  outChannelMapping = 0;
}

void InstructionZeroRegisterFormat::readCMT() {
  if (outChannel != NO_CHANNEL)
    core->readCMT(outChannel);
  else
    finished.notify(sc_core::SC_ZERO_TIME);
}

void InstructionZeroRegisterFormat::readCMTCallback(EncodedCMTEntry value) {
  outChannelMapping = value;
  finished.notify(sc_core::SC_ZERO_TIME);
}

// | p |   opcode    |  reg 1  | x |           immediate           |
//   2        7           5      2                 16
InstructionOneRegisterNoChannelFormat::InstructionOneRegisterNoChannelFormat(Instruction encoded) :
    InstructionBase(encoded),
    reg1(encoded.getBits(18, 22)),
    immediate(signExtend(encoded.getBits(0, 15), 16)) {
  // Nothing
}

// | p |   opcode    |  reg 1  |channel|         immediate         |
//   2        7           5        4                 14
InstructionOneRegisterFormat::InstructionOneRegisterFormat(Instruction encoded) :
    InstructionZeroRegisterFormat(encoded),
    reg1(encoded.getBits(18, 22)) {
  // Nothing
}

// | p |   opcode    |  reg 1  | xxxx  |  reg 2  |    immediate    |
//   2        7           5        4        5             9
InstructionTwoRegisterNoChannelFormat::InstructionTwoRegisterNoChannelFormat(Instruction encoded) :
    InstructionBase(encoded),
    reg1(encoded.getBits(18, 22)),
    reg2(encoded.getBits(9, 13)),
    immediate(signExtend(encoded.getBits(0, 8), 9)) {
  // Nothing
}

// | p |   opcode    |  reg 1  |channel|  reg 2  |    immediate    |
//   2        7           5        4        5             9
InstructionTwoRegisterFormat::InstructionTwoRegisterFormat(Instruction encoded) :
    InstructionTwoRegisterNoChannelFormat(encoded),
    outChannel(encoded.getBits(14, 17)) {
  outChannelMapping = 0;
}

void InstructionTwoRegisterFormat::readCMT() {
  if (outChannel != NO_CHANNEL)
    core->readCMT(outChannel);
  else
    finished.notify(sc_core::SC_ZERO_TIME);
}

void InstructionTwoRegisterFormat::readCMTCallback(EncodedCMTEntry value) {
  outChannelMapping = value;
  finished.notify(sc_core::SC_ZERO_TIME);
}

// | p |   opcode    |  reg 1  |channel|  reg 2  | xxxx  |immediate|
//   2        7           5        4        5        4        5
InstructionTwoRegisterShiftFormat::InstructionTwoRegisterShiftFormat(Instruction encoded) :
    InstructionTwoRegisterFormat(encoded) {
  // Would prefer to just extract the necessary bits, in case the don't care
  // bits are used for something else.
  assert(immediate < 32);
}

// | p |   opcode    |  reg 1  |channel|  reg 2  |  reg 3  |  fn   |
//   2        7           5        4        5         5        4
InstructionThreeRegisterFormat::InstructionThreeRegisterFormat(Instruction encoded) :
    InstructionBase(encoded),
    reg1(encoded.getBits(18, 22)),
    reg2(encoded.getBits(9, 13)),
    reg3(encoded.getBits(4, 8)),
    function(static_cast<function_t>(encoded.getBits(0, 3))),
    outChannel(encoded.getBits(14, 17)) {
  outChannelMapping = 0;
}

void InstructionThreeRegisterFormat::readCMT() {
  if (outChannel != NO_CHANNEL)
    core->readCMT(outChannel);
  else
    finished.notify(sc_core::SC_ZERO_TIME);
}

void InstructionThreeRegisterFormat::readCMTCallback(EncodedCMTEntry value) {
  outChannelMapping = value;
  finished.notify(sc_core::SC_ZERO_TIME);
}
