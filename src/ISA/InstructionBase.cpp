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

InstructionBase::~InstructionBase() {
  // Nothing
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
// therefore be an error.

void InstructionBase::readRegisters() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::readRegistersCallback(RegisterPort port, int32_t value) {
  assert(false);
}

void InstructionBase::writeRegisters() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::writeRegistersCallback() {assert(false);}

void InstructionBase::earlyCompute() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::earlyComputeCallback() {assert(false);}

void InstructionBase::compute() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::computeCallback() {assert(false);}

void InstructionBase::readCMT() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::readCMTCallback(EncodedCMTEntry value) {assert(false);}

void InstructionBase::writeCMT() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::writeCMTCallback() {assert(false);}

void InstructionBase::readScratchpad() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::readScratchpadCallback(int32_t value) {assert(false);}

void InstructionBase::writeScratchpad() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::writeScratchpadCallback() {assert(false);}

void InstructionBase::readCregs() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::readCregsCallback(int32_t value) {assert(false);}

void InstructionBase::writeCregs() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::writeCregsCallback() {assert(false);}

void InstructionBase::readPredicate() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::readPredicateCallback(bool value) {assert(false);}

void InstructionBase::writePredicate() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::writePredicateCallback() {assert(false);}

void InstructionBase::sendNetworkData() {finished.notify(sc_core::SC_ZERO_TIME);}
void InstructionBase::sendNetworkDataCallback() {assert(false);}

int32_t InstructionBase::signExtend(int32_t value, size_t bits) const {
  if (ISA::hasSignedImmediate(opcode))
    return (value << (32 - bits)) >> (32 - bits);
  else
    return value;
}
