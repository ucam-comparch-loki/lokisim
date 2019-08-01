/*
 * PipelineStage.h
 *
 * The base class for all pipeline stages.
 *
 *  Created on: 7 Jan 2010
 *      Author: db434
 */

#ifndef PIPELINESTAGE_H_
#define PIPELINESTAGE_H_

#include "../../Datatype/DecodedInst.h"
#include "../../LokiComponent.h"
#include "PipelineRegister.h"

class Core;
class PipelineRegister;

class PipelineStageBase : public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  // Clock.
  ClockInput clock;

//============================================================================//
// Constructors and destructors
//============================================================================//

protected:

  SC_HAS_PROCESS(PipelineStageBase);
  PipelineStageBase(const sc_module_name& name);

//============================================================================//
// Methods
//============================================================================//

public:

  // Inspect the instruction currently in this pipeline stage.
  const DecodedInst& currentInstruction() const;

  // Return the ID of this core.
  const ComponentID& id() const;

protected:

  // Do I want a parent() method, so the user has to know the module hierarchy,
  // but can access any position in it, or a core() method, which hides
  // any changes, but makes arbitrary access harder?
  Core& core() const;

  // The main loop responsible for this pipeline stage. It should execute
  // appropriate methods at appropriate points in each clock cycle.
  virtual void execute() = 0;

  // Recompute whether this pipeline stage is ready for new input.
  virtual void updateReady() = 0;
  virtual bool isStalled() const;

  // Obtain the next instruction to work on, and put it in the currentInst
  // field.
  virtual void prepareNextInstruction();

  // Signal that this pipeline stage has finished work on the current
  // instruction. This is separate from outputInstruction, above, as it is
  // possible to produce multiple outputs from a single instruction (stw).
  void instructionCompleted();

  // Receive an instruction from the previous stage.
  virtual DecodedInst receiveInstruction() = 0;

  // Send the transformed instruction on to the next pipeline stage.
  virtual void outputInstruction(const DecodedInst& inst) = 0;

  virtual bool previousStageBlocked() const = 0;
  virtual const sc_event& previousStageUnblockedEvent() const = 0;

  virtual bool nextStageBlocked() const = 0;
  virtual const sc_event& nextStageUnblockedEvent() const = 0;

  // Discard the next instruction to be executed, which is currently waiting in
  // a pipeline register. Returns whether anything was discarded.
  virtual bool discardNextInst() = 0;

//============================================================================//
// Local state
//============================================================================//

protected:

  // The instruction currently being worked on.
  DecodedInst currentInst;
  bool currentInstValid;

  sc_event newInstructionEvent, instructionCompletedEvent;

private:

//  PipelineRegister *prev, *next;

};


// A pipeline stage with only successors.
class FirstPipelineStage : public PipelineStageBase {

public:

  InstructionOutput nextStage;

  FirstPipelineStage(const sc_module_name& name);

  virtual DecodedInst receiveInstruction();
  virtual bool discardNextInst();
  virtual void outputInstruction(const DecodedInst& inst);
  virtual bool previousStageBlocked() const;
  virtual const sc_event& previousStageUnblockedEvent() const;
  virtual bool nextStageBlocked() const;
  virtual const sc_event& nextStageUnblockedEvent() const;

};


// A pipeline stage with only predecessors.
class LastPipelineStage : public PipelineStageBase {

public:

  InstructionInput previousStage;

  LastPipelineStage(const sc_module_name& name);

  virtual DecodedInst receiveInstruction();
  virtual bool discardNextInst();
  virtual void outputInstruction(const DecodedInst& inst);
  virtual bool previousStageBlocked() const;
  virtual const sc_event& previousStageUnblockedEvent() const;
  virtual bool nextStageBlocked() const;
  virtual const sc_event& nextStageUnblockedEvent() const;

};


// The default: a pipeline stage with both predecessors and successors.
// Would prefer to inherit behaviour from both FirstPipelineStage and
// LastPipelineStage, but that requires virtual inheritance, which breaks
// SystemC.
class PipelineStage : public PipelineStageBase {

public:

  InstructionInput previousStage;
  InstructionOutput nextStage;

  PipelineStage(const sc_module_name& name);

  virtual DecodedInst receiveInstruction();
  virtual bool discardNextInst();
  virtual void outputInstruction(const DecodedInst& inst);
  virtual bool previousStageBlocked() const;
  virtual const sc_event& previousStageUnblockedEvent() const;
  virtual bool nextStageBlocked() const;
  virtual const sc_event& nextStageUnblockedEvent() const;

};

#endif /* PIPELINESTAGE_H_ */
