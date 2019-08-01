/*
 * PipelineStage.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "PipelineStage.h"

#include "Core.h"
#include "PipelineRegister.h"
#include "../../Utility/Assert.h"

PipelineStageBase::PipelineStageBase(const sc_module_name& name) :
    LokiComponent(name),
    clock("clock") {

  currentInstValid = false;

  SC_METHOD(prepareNextInstruction);
  sensitive << clock.pos();
  dont_initialize();

}

const DecodedInst& PipelineStageBase::currentInstruction() const {
  return currentInst;
}

const ComponentID& PipelineStageBase::id() const {
  return core().id;
}

Core& PipelineStageBase::core() const {
  return static_cast<Core&>(*(this->get_parent_object()));
}

bool PipelineStageBase::isStalled() const {
  return false;
}

void PipelineStageBase::prepareNextInstruction() {
  currentInstValid = false;

  // Wait for both pipeline registers to be ready, and for the beginning of
  // a new clock cycle.
  if (previousStageBlocked())
    next_trigger(previousStageUnblockedEvent());
  else if (nextStageBlocked())
    next_trigger(nextStageUnblockedEvent());
  else if (!clock.posedge() || isStalled())
    next_trigger(clock.posedge_event());
  else {
    currentInst = receiveInstruction();
    currentInstValid = true;
    newInstructionEvent.notify(sc_core::SC_ZERO_TIME);

    LOKI_LOG(2) << this->name() << " received Instruction: " << currentInst << endl;

    // Wait until the pipeline stage finishes this instruction before retrieving
    // the next one.
    next_trigger(instructionCompletedEvent);
  }
}

void PipelineStageBase::instructionCompleted() {
  // Ensure that the instruction takes a finite amount of time to complete.
  // If it was able to complete instantly, it would be possible to execute
  // multiple instructions in the same cycle.
  instructionCompletedEvent.notify(sc_core::SC_ZERO_TIME);
}



FirstPipelineStage::FirstPipelineStage(const sc_module_name& name) :
    PipelineStageBase(name),
    nextStage("next") {
  // Nothing
}

void FirstPipelineStage::outputInstruction(const DecodedInst& inst) {
  nextStage->write(inst);
}

bool FirstPipelineStage::nextStageBlocked() const {
  return !nextStage->canWrite();
}

const sc_event& FirstPipelineStage::nextStageUnblockedEvent() const {
  return nextStage->canWriteEvent();
}

DecodedInst FirstPipelineStage::receiveInstruction() {
  loki_assert(false);
  return DecodedInst();
}

bool FirstPipelineStage::discardNextInst() {
  return false;
}

bool FirstPipelineStage::previousStageBlocked() const {
  return false;
}

const sc_event& FirstPipelineStage::previousStageUnblockedEvent() const {
  loki_assert(false);
  return *(new sc_event());
}



LastPipelineStage::LastPipelineStage(const sc_module_name& name) :
    PipelineStageBase(name),
    previousStage("prev") {
  // Nothing
}

void LastPipelineStage::outputInstruction(const DecodedInst& inst) {
  loki_assert(false);
}

bool LastPipelineStage::nextStageBlocked() const {
  return false;
}

const sc_event& LastPipelineStage::nextStageUnblockedEvent() const {
  loki_assert(false);
  return *(new sc_event());
}

DecodedInst LastPipelineStage::receiveInstruction() {
  return previousStage->read();
}

bool LastPipelineStage::discardNextInst() {
  return previousStage->discard();
}

bool LastPipelineStage::previousStageBlocked() const {
  return !previousStage->canRead();
}

const sc_event& LastPipelineStage::previousStageUnblockedEvent() const {
  return previousStage->canReadEvent();
}



PipelineStage::PipelineStage(const sc_module_name& name) :
    PipelineStageBase(name),
    previousStage("prev"),
    nextStage("next") {
  // Nothing
}

void PipelineStage::outputInstruction(const DecodedInst& inst) {
  nextStage->write(inst);
}

bool PipelineStage::nextStageBlocked() const {
  return !nextStage->canWrite();
}

const sc_event& PipelineStage::nextStageUnblockedEvent() const {
  return nextStage->canWriteEvent();
}

DecodedInst PipelineStage::receiveInstruction() {
  return previousStage->read();
}

bool PipelineStage::discardNextInst() {
  return previousStage->discard();
}

bool PipelineStage::previousStageBlocked() const {
  return !previousStage->canRead();
}

const sc_event& PipelineStage::previousStageUnblockedEvent() const {
  return previousStage->canReadEvent();
}
