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

class Core;
class PipelineRegister;

class PipelineStage : public LokiComponent {

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

  SC_HAS_PROCESS(PipelineStage);
  PipelineStage(const sc_module_name& name);

//============================================================================//
// Methods
//============================================================================//

public:

  // Inspect the instruction currently in this pipeline stage.
  const DecodedInst& currentInstruction() const;

  // Provide access to the surrounding pipeline registers. Either of these can
  // be NULL - used at the ends of the pipeline.
  void initPipeline(PipelineRegister* prev, PipelineRegister* next);

  // Return the ID of this core.
  const ComponentID& id() const;

protected:

  // Do I want a parent() method, so the user has to know the module hierarchy,
  // but can access any position in it, or a cluster() method, which hides
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
  virtual void getNextInstruction();

  // Send the transformed instruction on to the next pipeline stage.
  void outputInstruction(const DecodedInst& inst);

  // Signal that this pipeline stage has finished work on the current
  // instruction. This is separate from outputInstruction, above, as it is
  // possible to produce multiple outputs from a single instruction (stw).
  void instructionCompleted();

  virtual bool canReceiveInstruction() const;
  virtual bool canSendInstruction() const;

  // Immediately after these events have been triggered, the outputs of the
  // two methods above are known to be true.
  const sc_event& canReceiveEvent() const;
  const sc_event& canSendEvent() const;

  // Discard the next instruction to be executed, which is currently waiting in
  // a pipeline register. Returns whether anything was discarded.
  bool discardNextInst();

//============================================================================//
// Local state
//============================================================================//

protected:

  // The instruction currently being worked on.
  DecodedInst currentInst;
  bool currentInstValid;

  sc_event newInstructionEvent, instructionCompletedEvent;

private:

  PipelineRegister *prev, *next;

};

#endif /* PIPELINESTAGE_H_ */
