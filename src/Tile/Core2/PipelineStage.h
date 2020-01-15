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
// Constructors and destructors
//============================================================================//
protected:
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

  // TODO: some sort of mainLoop

  // Tasks to be carried out by each pipeline stage.
  virtual void execute() = 0;

//============================================================================//
// Local state
//============================================================================//

protected:

  friend class InstructionHandler;

  DecodedInstruction instruction;
};

} // end namespace

#endif /* SRC_TILE_CORE_PIPELINESTAGE_H_ */
