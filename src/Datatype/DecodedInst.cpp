/*
 * DecodedInst.cpp
 *
 *  Created on: 11 Sep 2010
 *      Author: daniel
 */

#include "DecodedInst.h"
#include "Instruction.h"
#include "MemoryRequest.h"
#include "../Utility/InstructionMap.h"

const uint8_t DecodedInst::operation() const {
  return operation_;
}

const RegisterIndex DecodedInst::sourceReg1() const {
  return sourceReg1_;
}

const RegisterIndex DecodedInst::sourceReg2() const {
  return sourceReg2_;
}

const RegisterIndex DecodedInst::destination() const {
  return destReg_;
}

const int32_t DecodedInst::immediate() const {
  return immediate_;
}

const ChannelIndex DecodedInst::channelMapEntry() const {
  return channelMapEntry_;
}

const uint8_t DecodedInst::predicate() const {
  return predicate_;
}

const bool    DecodedInst::setsPredicate() const {
  return setsPred_;
}

const uint8_t DecodedInst::memoryOp() const {
  return memoryOp_;
}


const int32_t DecodedInst::operand1() const {
  return operand1_;
}

const int32_t DecodedInst::operand2() const {
  return operand2_;
}

const int64_t DecodedInst::result() const {
  return result_;
}

const Address DecodedInst::location() const {
  return location_;
}


const bool    DecodedInst::usesPredicate() const {
  return (predicate_ == Instruction::NOT_P) || (predicate_ == Instruction::P);
}

const bool    DecodedInst::hasOperand1() const {
  return hasOperand1_;
}

const bool    DecodedInst::hasResult() const {
  return hasResult_;
}


const bool    DecodedInst::hasImmediate() const {
  return InstructionMap::hasImmediate(operation_);
}

const bool    DecodedInst::isALUOperation() const {
  return InstructionMap::isALUOperation(operation_);
}

const std::string& DecodedInst::name() const {
  return InstructionMap::name(operation_);
}


void    DecodedInst::operation(const uint8_t val) {
  operation_ = val;
}

void    DecodedInst::sourceReg1(const RegisterIndex val) {
  sourceReg1_ = val;
}

void    DecodedInst::sourceReg2(const RegisterIndex val) {
  sourceReg2_ = val;
}

void    DecodedInst::destination(const RegisterIndex val) {
  destReg_ = val;
}

void    DecodedInst::immediate(const int32_t val) {
  immediate_ = val;
}

void    DecodedInst::channelMapEntry(const ChannelIndex val) {
  channelMapEntry_ = val;
}

void    DecodedInst::predicate(const uint8_t val) {
  predicate_ = val;
}

void    DecodedInst::setsPredicate(const bool val) {
  setsPred_ = val;
}

void    DecodedInst::memoryOp(const uint8_t val) {
  memoryOp_ = val;
}


void    DecodedInst::operand1(const int32_t val) {
  operand1_ = val;
}

void    DecodedInst::operand2(const int32_t val) {
  operand2_ = val;
}

void    DecodedInst::result(const int64_t val) {
  result_ = val;
  hasResult_ = true;
}

void    DecodedInst::location(const Address val) {
  location_ = val;
}


Instruction DecodedInst::toInstruction() const {
  Instruction i;
  i.opcode(InstructionMap::opcode(name()));
  i.destination(destReg_);
  i.sourceReg1(sourceReg1_);
  i.sourceReg2(sourceReg2_);
  i.immediate(immediate_);
  i.remoteChannel(channelMapEntry_);
  i.predicate(predicate_);
  i.setsPredicate(setsPred_);
  return i;
}


bool DecodedInst::operator== (const DecodedInst& other) const {
  return  operation_       == other.operation_       &&
          sourceReg1_      == other.sourceReg1_      &&
          sourceReg2_      == other.sourceReg2_      &&
          destReg_         == other.destReg_         &&
          immediate_       == other.immediate_       &&
          channelMapEntry_ == other.channelMapEntry_ &&
          predicate_       == other.predicate_       &&
          setsPred_        == other.setsPred_        &&

          operand1_        == other.operand1_        &&
          operand2_        == other.operand2_        &&
          result_          == other.result_          &&

          hasOperand1_     == other.hasOperand1_     &&
          hasResult_       == other.hasResult_       &&

          location_        == other.location_;
}

DecodedInst& DecodedInst::operator= (const DecodedInst& other) {
  operation_       = other.operation_;
  sourceReg1_      = other.sourceReg1_;
  sourceReg2_      = other.sourceReg2_;
  destReg_         = other.destReg_;
  immediate_       = other.immediate_;
  channelMapEntry_ = other.channelMapEntry_;
  predicate_       = other.predicate_;
  setsPred_        = other.setsPred_;
  memoryOp_        = other.memoryOp_;

  operand1_        = other.operand1_;
  operand2_        = other.operand2_;
  result_          = other.result_;

  hasOperand1_     = other.hasOperand1_;
  hasResult_       = other.hasResult_;

  location_        = other.location_;

  return *this;
}

std::ostream& DecodedInst::print(std::ostream& os) const {
  if(predicate() == Instruction::P) os << "p?";
  else if(predicate() == Instruction::NOT_P) os << "!p?";

  os << name()
     << (setsPredicate()?".p":"") << (predicate()==Instruction::END_OF_PACKET?".eop":"")
     << " r" << (int)destination() << " r" << (int)sourceReg1()
     << " r" << (int)sourceReg2()     << " "  << immediate();
  if(channelMapEntry() != Instruction::NO_CHANNEL)
    os << " -> " << (int)channelMapEntry();

  return os;
}


DecodedInst::DecodedInst() {
  // Everything should default to 0.
}

DecodedInst::DecodedInst(const Instruction i) {
  operation_       = InstructionMap::operation(i.opcode());
  sourceReg1_      = i.sourceReg1();
  sourceReg2_      = i.sourceReg2();
  destReg_         = i.destination();
  immediate_       = i.immediate();
  channelMapEntry_ = i.remoteChannel();
  predicate_       = i.predicate();
  setsPred_        = i.setsPredicate();
  memoryOp_        = MemoryRequest::NONE;

  operand1_        = 0;
  operand2_        = 0;
  result_          = 0;

  hasOperand1_ = hasResult_ = false;
}

DecodedInst::~DecodedInst() {

}
