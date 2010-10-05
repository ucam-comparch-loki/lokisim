/*
 * DecodedInst.cpp
 *
 *  Created on: 11 Sep 2010
 *      Author: daniel
 */

#include "DecodedInst.h"
#include "MemoryRequest.h"

uint8_t DecodedInst::operation() const {
  return operation_;
}

uint8_t DecodedInst::sourceReg1() const {
  return sourceReg1_;
}

uint8_t DecodedInst::sourceReg2() const {
  return sourceReg2_;
}

uint8_t DecodedInst::destinationReg() const {
  return destReg_;
}

int32_t DecodedInst::immediate() const {
  return immediate_;
}

uint8_t DecodedInst::channelMapEntry() const {
  return channelMapEntry_;
}

uint8_t DecodedInst::predicate() const {
  return predicate_;
}

bool    DecodedInst::setsPredicate() const {
  return setsPred_;
}

uint8_t DecodedInst::memoryOp() const {
  return memoryOp_;
}


int32_t DecodedInst::operand1() const {
  return operand1_;
}

int32_t DecodedInst::operand2() const {
  return operand2_;
}

int64_t DecodedInst::result() const {
  return result_;
}


bool    DecodedInst::hasOperand1() const {
  return hasOperand1_;
}

bool    DecodedInst::hasResult() const {
  return hasResult_;
}


void    DecodedInst::operation(const uint8_t val) {
  operation_ = val;
}

void    DecodedInst::sourceReg1(const uint8_t val) {
  sourceReg1_ = val;
}

void    DecodedInst::sourceReg2(const uint8_t val) {
  sourceReg2_ = val;
}

void    DecodedInst::destination(const uint8_t val) {
  destReg_ = val;
}

void    DecodedInst::immediate(const int32_t val) {
  immediate_ = val;
}

void    DecodedInst::channelMapEntry(const uint8_t val) {
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


bool DecodedInst::operator== (const DecodedInst& other) const {
  return  operation_       == other.operation_ &&
          sourceReg1_      == other.sourceReg1_ &&
          sourceReg2_      == other.sourceReg2_ &&
          destReg_         == other.destReg_ &&
          immediate_       == other.immediate_ &&
          channelMapEntry_ == other.channelMapEntry_ &&
          predicate_       == other.predicate_ &&
          setsPred_        == other.setsPred_ &&

          operand1_        == other.operand1_ &&
          operand2_        == other.operand2_ &&
          result_          == other.result_ &&

          hasOperand1_     == other.hasOperand1_ &&
          hasResult_       == other.hasResult_;
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

  return *this;
}


DecodedInst::DecodedInst() {
  // Everything should default to 0.
}

DecodedInst::DecodedInst(Instruction i) {
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
