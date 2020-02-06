/*
 * PipelineStage.cpp
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#include "PipelineStage.h"
#include "Core.h"
#include "../../Utility/Assert.h"

namespace Compute {

PipelineStage::PipelineStage(sc_module_name name) :
    LokiComponent(name),
    CoreInterface(),
    clock("clock") {

  SC_METHOD(mainLoop);
  // do initialise
}

void PipelineStage::readRegister(RegisterIndex index, PortIndex port) {
  LOKI_ERROR << "Can't read registers from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::writeRegister(RegisterIndex index, int32_t value) {
  LOKI_ERROR << "Can't write registers from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::readPredicate() {
  LOKI_ERROR << "Can't read predicate from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::writePredicate(bool value) {
  LOKI_ERROR << "Can't write predicate from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::fetch(MemoryAddr address,
                           ChannelMapEntry::MemoryChannel channel,
                           bool execute, bool persistent) {
  LOKI_ERROR << "Can't fetch from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::jump(JumpOffset offset) {
  LOKI_ERROR << "Can't jump from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::computeLatency(opcode_t opcode, function_t fn) {
  LOKI_ERROR << "Can't compute in " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::readCMT(RegisterIndex index) {
  LOKI_ERROR << "Can't read channel map table from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::writeCMT(RegisterIndex index, EncodedCMTEntry value) {
  LOKI_ERROR << "Can't write channel map table from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::readCreg(RegisterIndex index) {
  LOKI_ERROR << "Can't read control registers from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::writeCreg(RegisterIndex index, int32_t value) {
  LOKI_ERROR << "Can't write control registers from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::readScratchpad(RegisterIndex index) {
  LOKI_ERROR << "Can't read scratchpad from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::writeScratchpad(RegisterIndex index, int32_t value) {
  LOKI_ERROR << "Can't write scratchpad from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::syscall(int code) {
  LOKI_ERROR << "Can't make system calls from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::sendOnNetwork(NetworkData flit) {
  LOKI_ERROR << "Can't send on network from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::selectChannelWithData(uint bitmask) {
  LOKI_ERROR << "Can't select input channel from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::waitForCredit(ChannelIndex channel) {
  LOKI_ERROR << "Can't wait for credits from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::startRemoteExecution(ChannelID address) {
  LOKI_ERROR << "Can't start remote execution from " << this->name() << endl;
  loki_assert(false);
}

void PipelineStage::endRemoteExecution() {
  LOKI_ERROR << "Can't end remote execution from " << this->name() << endl;
  loki_assert(false);
}

ChannelID PipelineStage::getNetworkDestination(EncodedCMTEntry channelMap,
                                               MemoryAddr address) const {
  return core().getNetworkDestination(channelMap, address);
}

bool PipelineStage::inputFIFOHasData(ChannelIndex fifo) const {
  return core().inputFIFOHasData(fifo);
}

uint PipelineStage::creditsAvailable(ChannelIndex channel) const {
  return core().creditsAvailable(channel);
}

Core& PipelineStage::core() const {
  return static_cast<Core&>(*(this->get_parent_object()));
}

void PipelineStage::mainLoop() {

  // Wait for instruction to be available.
  if (previousStageBlocked())
    next_trigger(previousStageUnblockedEvent());
  // Wait until pipeline unstalls.
  else if (nextStageBlocked())
    next_trigger(nextStageUnblockedEvent());
  // Start executing a new instruction.
  else if (!instruction) {
    // TODO: check that there is an instruction to receive?
    // Is this part of previousStageBlocked()?
    instruction = receiveInstruction();
    // TODO: register instruction to this layer? instruction.assignToCore(*this)?
    // Need to remove memory address and instruction source from arguments.

    LOKI_LOG(2) << this->name() << " received instruction: " << instruction << endl;

    execute();

    // Instruction may do nothing in this stage, so can't naively check
    // instruction->finishedPhaseEvent.
    next_trigger(sc_core::SC_ZERO_TIME);
  }
  // Wait until all phases of the instruction are finished (for this stage).
  else if (instruction->busy())
    next_trigger(instruction->finishedPhaseEvent());
  // Send the instruction to the next stage and delete the local copy.
  else {
    sendInstruction(instruction);
    instruction.reset();

    // Trigger? Default is next clock edge, which seems reasonable.
  }

}

DecodedInstruction PipelineStage::receiveInstruction() {
  loki_assert(false);
  return DecodedInstruction();
}

void PipelineStage::sendInstruction(const DecodedInstruction inst) {
  loki_assert(false);
}

bool PipelineStage::previousStageBlocked() const {
  return false;
}

const sc_event& PipelineStage::previousStageUnblockedEvent() const {
  loki_assert(false);
  return *(new sc_event());
}

bool PipelineStage::nextStageBlocked() const {
  return false;
}

const sc_event& PipelineStage::nextStageUnblockedEvent() const {
  loki_assert(false);
  return *(new sc_event());
}

bool PipelineStage::discardNextInst() {
  return false;
}



template<class T>
HasSucceedingStage<T>::HasSucceedingStage(const sc_module_name& name) :
    T(name),
    nextStage("output") {
  // Nothing
}

template<class T>
void HasSucceedingStage<T>::sendInstruction(const DecodedInstruction inst) {
  nextStage->write(inst);
}

template<class T>
bool HasSucceedingStage<T>::nextStageBlocked() const {
  return !nextStage->canWrite();
}

template<class T>
const sc_event& HasSucceedingStage<T>::nextStageUnblockedEvent() const {
  return nextStage->canWriteEvent();
}



template<class T>
HasPrecedingStage<T>::HasPrecedingStage(const sc_module_name& name) :
    T(name),
    previousStage("input") {
  // Nothing
}

template<class T>
DecodedInstruction HasPrecedingStage<T>::receiveInstruction() {
  return previousStage->read();
}

template<class T>
bool HasPrecedingStage<T>::previousStageBlocked() const {
  return !previousStage->canRead();
}

template<class T>
const sc_event& HasPrecedingStage<T>::previousStageUnblockedEvent() const {
  return previousStage->canReadEvent();
}

template<class T>
bool HasPrecedingStage<T>::discardNextInst() {
  return previousStage->discard();
}

} // end namespace
