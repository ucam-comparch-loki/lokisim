/*
 * InstructionBase.cpp
 *
 *  Created on: 2 Aug 2019
 *      Author: db434
 */

#include "InstructionBase.h"

InstructionBase::InstructionBase(Instruction encoded) :
    encoded(encoded),
    opcode(encoded.opcode()),
    predicate(encoded.predicate()) {

  core = NULL;
  location = -1;
  source = UNKNOWN;

}

// Prepare the instruction for execution on a given core. Also provide some
// extra information to help with debugging.
void InstructionBase::assignToCore(CoreInterface& core, MemoryAddr address,
                                   InstructionSource source) {
  this->core = &core;
  this->location = address;
  this->source = source;
}

// Event triggered whenever a phase of computation completes. At most one
// phase may be in progress at a time.
const sc_core::sc_event& InstructionBase::finishedPhaseEvent() const {
  return finished;
}

// The default for all phases is to do nothing. Receiving a callback should
// therefore be an error. The core may be waiting for `finishedPhaseEvent()`, so
// need to make a point of starting and immediately finishing each phase.

void InstructionBase::readRegisters() {
  startingPhase(INST_REG_READ);
  finishedPhase(INST_REG_READ);
}
void InstructionBase::readRegistersCallback(RegisterPort port, int32_t value) {
  assert(false);
}

void InstructionBase::writeRegisters() {
  startingPhase(INST_REG_WRITE);
  finishedPhase(INST_REG_WRITE);
}
void InstructionBase::writeRegistersCallback() {assert(false);}

void InstructionBase::earlyCompute() {
  startingPhase(INST_COMPUTE);
  finishedPhase(INST_COMPUTE);
}
void InstructionBase::earlyComputeCallback(int32_t value) {assert(false);}

void InstructionBase::compute() {
  startingPhase(INST_COMPUTE);
  finishedPhase(INST_COMPUTE);
}
void InstructionBase::computeCallback(int32_t value) {assert(false);}

void InstructionBase::readCMT() {
  startingPhase(INST_CMT_READ);
  finishedPhase(INST_CMT_READ);
}
void InstructionBase::readCMTCallback(EncodedCMTEntry value) {assert(false);}

void InstructionBase::writeCMT() {
  startingPhase(INST_CMT_WRITE);
  finishedPhase(INST_CMT_WRITE);
}
void InstructionBase::writeCMTCallback() {assert(false);}

void InstructionBase::readScratchpad() {
  startingPhase(INST_SPAD_READ);
  finishedPhase(INST_SPAD_READ);
}
void InstructionBase::readScratchpadCallback(int32_t value) {assert(false);}

void InstructionBase::writeScratchpad() {
  startingPhase(INST_SPAD_WRITE);
  finishedPhase(INST_SPAD_WRITE);
}
void InstructionBase::writeScratchpadCallback() {assert(false);}

void InstructionBase::readCregs() {
  startingPhase(INST_CREG_READ);
  finishedPhase(INST_CREG_READ);
}
void InstructionBase::readCregsCallback(int32_t value) {assert(false);}

void InstructionBase::writeCregs() {
  startingPhase(INST_CREG_WRITE);
  finishedPhase(INST_CREG_WRITE);
}
void InstructionBase::writeCregsCallback() {assert(false);}

void InstructionBase::readPredicate() {
  startingPhase(INST_PRED_READ);
  finishedPhase(INST_PRED_READ);
}
void InstructionBase::readPredicateCallback(bool value) {assert(false);}

void InstructionBase::writePredicate() {
  startingPhase(INST_PRED_WRITE);
  finishedPhase(INST_PRED_WRITE);
}
void InstructionBase::writePredicateCallback() {assert(false);}

void InstructionBase::sendNetworkData() {
  startingPhase(INST_NETWORK_SEND);
  finishedPhase(INST_NETWORK_SEND);
}
void InstructionBase::sendNetworkDataCallback() {assert(false);}

int32_t InstructionBase::signExtend(int32_t value, size_t bits) const {
  if (ISA::hasSignedImmediate(opcode))
    return (value << (32 - bits)) >> (32 - bits);
  else
    return value;
}

bool InstructionBase::busy() const {
  return phasesInProgress.any();
}

void InstructionBase::startingPhase(ExecutionPhase phase) {
  assert(phasesInProgress[phase] == false);
  phasesInProgress[phase] = true;
}

void InstructionBase::finishedPhase(ExecutionPhase phase) {
  assert(phasesInProgress[phase] == true);
  phasesInProgress[phase] = false;

  finished.notify(sc_core::SC_ZERO_TIME);
}

bool InstructionBase::phaseInProgress(ExecutionPhase phase) {
  return phasesInProgress.test(phase);
}
