/*
 * PipelineStage.h
 *
 * Each pipeline stage implements a subset of the CoreInterface. This class
 * provides sensible defaults for the remaining methods.
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_PIPELINESTAGE_H_
#define SRC_TILE_CORE_PIPELINESTAGE_H_

#include "../../ISA/CoreInterface.h"
#include "../../ISA/InstructionDecode.h"
#include "../../LokiComponent.h"

namespace Compute {

class Core;

class PipelineStage : public LokiComponent, public CoreInterface {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput clock;

//============================================================================//
// Constructors and destructors
//============================================================================//

protected:

  SC_HAS_PROCESS(PipelineStage);
  PipelineStage(sc_module_name name);

//============================================================================//
// Methods
//============================================================================//

public:

  virtual void readRegister(RegisterIndex index, RegisterPort port);
  virtual void writeRegister(RegisterIndex index, int32_t value);

  virtual void readPredicate();
  virtual void writePredicate(bool value);

  virtual void fetch(MemoryAddr address, ChannelMapEntry::MemoryChannel channel,
                     bool execute, bool persistent);
  virtual void jump(JumpOffset offset);

  virtual void computeLatency(opcode_t opcode, function_t fn=(function_t)0);

  virtual void readCMT(RegisterIndex index);
  virtual void writeCMT(RegisterIndex index, EncodedCMTEntry value);

  virtual void readCreg(RegisterIndex index);
  virtual void writeCreg(RegisterIndex index, int32_t value);

  virtual void readScratchpad(RegisterIndex index);
  virtual void writeScratchpad(RegisterIndex index, int32_t value);

  virtual void syscall(int code);

  virtual void sendOnNetwork(NetworkData flit);
  virtual void selectChannelWithData(uint bitmask);
  virtual void waitForCredit(ChannelIndex channel);

  virtual void startRemoteExecution(ChannelID address);
  virtual void endRemoteExecution();

  virtual ChannelID getNetworkDestination(EncodedCMTEntry channelMap,
                                          MemoryAddr address=0) const;

  virtual bool inputFIFOHasData(ChannelIndex fifo) const;

  virtual uint creditsAvailable(ChannelIndex channel) const;

protected:

  Core& core() const;

  // Glue logic surrounding the main execution. e.g. Receiving/sending
  // instructions, stalling, etc.
  virtual void mainLoop();

  // Tasks to be carried out by each pipeline stage.
  virtual void execute() = 0;

  // Methods which handle supply of instructions and communication with
  // neighbouring pipeline stages. By default these do not work - they should
  // be overridden.
  DecodedInstruction receiveInstruction();
  void sendInstruction(const DecodedInstruction inst);
  bool previousStageBlocked() const;
  const sc_event& previousStageUnblockedEvent() const;
  bool nextStageBlocked() const;
  const sc_event& nextStageUnblockedEvent() const;

  // Discard the next instruction to be executed, which is currently waiting in
  // a pipeline register. Returns whether anything was discarded.
  bool discardNextInst();

//============================================================================//
// Local state
//============================================================================//

protected:

  friend class InstructionHandler;

  DecodedInstruction instruction;
};


// Mix-in to add a connection to a successor pipeline stage.
template<class T>
class HasSucceedingStage : public T {
public:
  InstructionOutput nextStage;

protected:
  HasSucceedingStage(const sc_module_name& name);

  void sendInstruction(const DecodedInstruction inst);
  bool nextStageBlocked() const;
  const sc_event& nextStageUnblockedEvent() const;

};


// Mix-in to add a connection to a preceding pipeline stage.
template<class T>
class HasPrecedingStage : public T {
public:
  InstructionInput previousStage;

protected:
  HasPrecedingStage(const sc_module_name& name);

  DecodedInstruction receiveInstruction();
  bool previousStageBlocked() const;
  const sc_event& previousStageUnblockedEvent() const;
  bool discardNextInst();

};


// Mix-ins allow code to be reused in MiddlePipelineStage without using multiple
// inheritance (which breaks SystemC).
typedef HasSucceedingStage<PipelineStage> FirstPipelineStage;
typedef HasPrecedingStage<PipelineStage>  LastPipelineStage;
typedef HasPrecedingStage<HasSucceedingStage<PipelineStage>> MiddlePipelineStage;

} // end namespace

#endif /* SRC_TILE_CORE_PIPELINESTAGE_H_ */
