/*
 * PipelineStage.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "PipelineStage.h"
#include "PipelineRegister.h"
#include "../Core.h"

void PipelineStage::initPipeline(PipelineRegister* prev, PipelineRegister* next) {
  this->prev = prev;
  this->next = next;
}

Core* PipelineStage::core() const {
  return static_cast<Core*>(this->get_parent());
}

bool PipelineStage::isStalled() const {
  return false;
}

void PipelineStage::getNextInstruction() {
  // Need to wait until the previous stage (if any) has passed us an
  // instruction, and until the next stage (if any) is ready to receive an
  // instruction.
  bool prevReady = (prev == NULL) || canReceiveInstruction();
  bool nextReady = (next == NULL) || canSendInstruction();

  currentInstValid = false;

  // Wait for both pipeline registers to be ready, and for the beginning of
  // a new clock cycle.
  if (!prevReady)
    next_trigger(prev->dataAdded());
  else if (!nextReady)
    next_trigger(next->dataRemoved());
  else if (!clock.posedge() || isStalled())
    next_trigger(clock.posedge_event());
  else {
    currentInst = prev->read();
    currentInstValid = true;
    newInstructionEvent.notify(sc_core::SC_ZERO_TIME);

    // Wait until the pipeline stage finishes this instruction before retrieving
    // the next one.
    next_trigger(instructionCompletedEvent);
  }
}

void PipelineStage::outputInstruction(const DecodedInst& inst) {
  assert(next != NULL);
  next->write(inst);
}

void PipelineStage::instructionCompleted() {
  // Ensure that the instruction takes a finite amount of time to complete.
  // If it was able to complete instantly, it would be possible to execute
  // multiple instructions in the same cycle.
  instructionCompletedEvent.notify(sc_core::SC_ZERO_TIME);
}

bool PipelineStage::canReceiveInstruction() const {
  return (prev != NULL) && prev->hasData();
}

bool PipelineStage::canSendInstruction() const {
  return (next != NULL) && next->ready();
}

const sc_event& PipelineStage::canReceiveEvent() const {
  assert(prev != NULL);
  return prev->dataAdded();
}

const sc_event& PipelineStage::canSendEvent() const {
  if (next == NULL)
    cout << this->name() << endl;
  assert(next != NULL);
  return next->dataRemoved();
}

bool PipelineStage::discardNextInst() {
  if (prev == NULL)
    return false;
  else
    return prev->discard();
}

PipelineStage::PipelineStage(const sc_module_name& name, const ComponentID& ID) :
    Component(name, ID) {

  currentInstValid = false;

  SC_METHOD(getNextInstruction);
  sensitive << clock.pos();
  dont_initialize();

}
