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

// Can this instruction be squashed based on the value of the predicate?
bool InstructionBase::isPredicated() const {
  return (predicate == EXECUTE_IF_P) || (predicate == EXECUTE_IF_NOT_P);
}

// Is this instruction the final one in its instruction packet?
bool InstructionBase::isEndOfPacket() const {
  return predicate == END_OF_PACKET;
}

// Does this instruction read from the register file?
bool InstructionBase::readsRegister(PortIndex port) const {return false;}

// Does this instruction write to the register file?
bool InstructionBase::writesRegister() const {return false;}

// Does this instruction read the predicate bit? (In order to compute a
// result, not to see if the instruction should execute or not.)
bool InstructionBase::readsPredicate() const {return false;}

// Does this instruction write the predicate bit?
bool InstructionBase::writesPredicate() const {return false;}

// Does this instruction read from the channel map table?
bool InstructionBase::readsCMT() const {return false;}

// Does this instruction write to the channel map table?
bool InstructionBase::writesCMT() const {return false;}

// Does this instruction read the control registers?
bool InstructionBase::readsCReg() const {return false;}

// Does this instruction write to the control registers?
bool InstructionBase::writesCReg() const {return false;}

// Does this instruction read from the scratchpad?
bool InstructionBase::readsScratchpad() const {return false;}

// Does this instruction write to the scratchpad?
bool InstructionBase::writesScratchpad() const {return false;}

// Does this instruction send data onto the network?
// (Fetches don't count - the network access is a side effect of checking the
// cache tags.)
bool InstructionBase::sendsOnNetwork() const {return false;}


// Return the index of the register read from the given port.
RegisterIndex InstructionBase::getSourceRegister(PortIndex port) const {
  assert(false);
  return 0;
}

// Return the index of the register written.
RegisterIndex InstructionBase::getDestinationRegister() const {
  assert(false);
  return 0;
}

// Return the computed result of the instruction.
int32_t InstructionBase::getResult() const {
  assert(false);
  return 0;
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
  startingPhase(InstructionInterface::INST_REG_READ);
  finishedPhase(InstructionInterface::INST_REG_READ);
}
void InstructionBase::readRegistersCallback(PortIndex port, int32_t value) {
  assert(false);
}

void InstructionBase::writeRegisters() {
  startingPhase(InstructionInterface::INST_REG_WRITE);
  finishedPhase(InstructionInterface::INST_REG_WRITE);
}
void InstructionBase::writeRegistersCallback(PortIndex port) {assert(false);}

void InstructionBase::earlyCompute() {
  startingPhase(InstructionInterface::INST_COMPUTE);
  finishedPhase(InstructionInterface::INST_COMPUTE);
}
void InstructionBase::earlyComputeCallback(int32_t value) {assert(false);}

void InstructionBase::compute() {
  startingPhase(InstructionInterface::INST_COMPUTE);
  finishedPhase(InstructionInterface::INST_COMPUTE);
}
void InstructionBase::computeCallback(int32_t value) {assert(false);}

void InstructionBase::readCMT() {
  startingPhase(InstructionInterface::INST_CMT_READ);
  finishedPhase(InstructionInterface::INST_CMT_READ);
}
void InstructionBase::readCMTCallback(PortIndex port, EncodedCMTEntry value) {
  assert(false);
}

void InstructionBase::writeCMT() {
  startingPhase(InstructionInterface::INST_CMT_WRITE);
  finishedPhase(InstructionInterface::INST_CMT_WRITE);
}
void InstructionBase::writeCMTCallback(PortIndex port) {assert(false);}

void InstructionBase::readScratchpad() {
  startingPhase(InstructionInterface::INST_SPAD_READ);
  finishedPhase(InstructionInterface::INST_SPAD_READ);
}
void InstructionBase::readScratchpadCallback(PortIndex port, int32_t value) {
  assert(false);
}

void InstructionBase::writeScratchpad() {
  startingPhase(InstructionInterface::INST_SPAD_WRITE);
  finishedPhase(InstructionInterface::INST_SPAD_WRITE);
}
void InstructionBase::writeScratchpadCallback(PortIndex port) {assert(false);}

void InstructionBase::readCregs() {
  startingPhase(InstructionInterface::INST_CREG_READ);
  finishedPhase(InstructionInterface::INST_CREG_READ);
}
void InstructionBase::readCregsCallback(PortIndex port, int32_t value) {
  assert(false);
}

void InstructionBase::writeCregs() {
  startingPhase(InstructionInterface::INST_CREG_WRITE);
  finishedPhase(InstructionInterface::INST_CREG_WRITE);
}
void InstructionBase::writeCregsCallback(PortIndex port) {assert(false);}

void InstructionBase::readPredicate() {
  startingPhase(InstructionInterface::INST_PRED_READ);

  // Some duplication with the ReadPredicate mix-in.
  // Does this cause conflicts for instructions which are predicated and read
  // the predicate for their computation?
  if (predicate == EXECUTE_IF_P || predicate == EXECUTE_IF_NOT_P)
    this->core->readPredicate();
  else
    finishedPhase(InstructionInterface::INST_PRED_READ);
}
void InstructionBase::readPredicateCallback(PortIndex port, bool value) {
  bool squashed = ((predicate == EXECUTE_IF_P)     && !value)
               || ((predicate == EXECUTE_IF_NOT_P) &&  value);

  // TODO: do something with this value.

  this->finishedPhase(InstructionInterface::INST_PRED_READ);
}

void InstructionBase::writePredicate() {
  startingPhase(InstructionInterface::INST_PRED_WRITE);
  finishedPhase(InstructionInterface::INST_PRED_WRITE);
}
void InstructionBase::writePredicateCallback(PortIndex port) {assert(false);}

void InstructionBase::sendNetworkData() {
  startingPhase(InstructionInterface::INST_NETWORK_SEND);
  finishedPhase(InstructionInterface::INST_NETWORK_SEND);
}
void InstructionBase::sendNetworkDataCallback() {assert(false);}

void InstructionBase::creditArrivedCallback() {assert(false);}

int32_t InstructionBase::signExtend(int32_t value, size_t bits) const {
  if (ISA::hasSignedImmediate(opcode))
    return (value << (32 - bits)) >> (32 - bits);
  else
    return value;
}

bool InstructionBase::busy() const {
  return phasesInProgress.any();
}

void InstructionBase::startingPhase(InstructionInterface::ExecutionPhase phase) {
  assert(phasesInProgress[phase] == false);
  phasesInProgress[phase] = true;
}

void InstructionBase::finishedPhase(InstructionInterface::ExecutionPhase phase) {
  assert(phasesInProgress[phase] == true);
  phasesInProgress[phase] = false;
  phasesComplete[phase] = true;

  finished.notify(sc_core::SC_ZERO_TIME);
}

bool InstructionBase::phaseInProgress(InstructionInterface::ExecutionPhase phase) const {
  return phasesInProgress.test(phase);
}

bool InstructionBase::completedPhase(InstructionInterface::ExecutionPhase phase) const {
  return phasesComplete.test(phase);
}
