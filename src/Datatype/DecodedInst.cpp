/*
 * DecodedInst.cpp
 *
 *  Created on: 11 Sep 2010
 *      Author: daniel
 */

#include "DecodedInst.h"
#include "Instruction.h"
#include "MemoryRequest.h"

uint8_t DecodedInst::getOperation() const {
  return operation;
}

uint8_t DecodedInst::getSource1() const {
  return sourceReg1;
}

uint8_t DecodedInst::getSource2() const {
  return sourceReg2;
}

uint8_t DecodedInst::getDestination() const {
  return destReg;
}

int32_t DecodedInst::getImmediate() const {
  return immediate;
}
uint8_t DecodedInst::getChannelMap() const {
  return channelMapEntry;
}

uint8_t DecodedInst::getPredicate() const {
  return predicate;
}

bool    DecodedInst::getSetPredicate() const {
  return setPred;
}

uint8_t DecodedInst::getMemoryOp() const {
  return memoryOp;
}


int32_t DecodedInst::getOperand1() const {
  return operand1;
}

int32_t DecodedInst::getOperand2() const {
  return operand2;
}

int64_t DecodedInst::getResult() const {
  return result;
}


bool    DecodedInst::hasOperand1() const {
  return _hasOperand1;
}

bool    DecodedInst::hasResult() const {
  return _hasResult;
}


void    DecodedInst::setOperation(uint8_t val) {
  operation = val;
}

void    DecodedInst::setSource1(uint8_t val) {
  sourceReg1 = val;
}

void    DecodedInst::setSource2(uint8_t val) {
  sourceReg2 = val;
}

void    DecodedInst::setDestination(uint8_t val) {
  destReg = val;
}

void    DecodedInst::setImmediate(int32_t val) {
  immediate = val;
}

void    DecodedInst::setChannelMap(uint8_t val) {
  channelMapEntry = val;
}

void    DecodedInst::setPredicate(uint8_t val) {
  predicate = val;
}

void    DecodedInst::setSetPredicate(bool val) {
  setPred = val;
}

void    DecodedInst::setMemoryOp(uint8_t val) {
  memoryOp = val;
}


void    DecodedInst::setOperand1(int32_t val) {
  operand1 = val;
}

void    DecodedInst::setOperand2(int32_t val) {
  operand2 = val;
}

void    DecodedInst::setResult(int64_t val) {
  result = val;
  _hasResult = true;
}


bool DecodedInst::operator== (const DecodedInst& other) const {
  return false;   // TODO
}

DecodedInst& DecodedInst::operator= (const DecodedInst& other) {
  operation       = other.operation;
  sourceReg1      = other.sourceReg1;
  sourceReg2      = other.sourceReg2;
  destReg         = other.destReg;
  immediate       = other.immediate;
  channelMapEntry = other.channelMapEntry;
  predicate       = other.predicate;
  setPred         = other.setPred;

  operand1        = other.operand1;
  operand2        = other.operand2;
  result          = other.result;

  _hasOperand1    = other._hasOperand1;
  _hasResult      = other._hasResult;

  return *this;
}


DecodedInst::DecodedInst() {
  // Everything should default to 0.
}

DecodedInst::DecodedInst(Instruction i) {
  operation       = i.getOp();
  sourceReg1      = i.getSrc1();
  sourceReg2      = i.getSrc2();
  destReg         = i.getDest();
  immediate       = i.getImmediate();
  channelMapEntry = i.getRchannel();
  predicate       = i.getPredicate();
  setPred         = i.getSetPredicate();
  memoryOp        = MemoryRequest::NONE;

  operand1        = 0;
  operand2        = 0;
  result          = 0;
}

DecodedInst::~DecodedInst() {

}
