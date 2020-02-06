/*
 * DecodeStage.h
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_DECODESTAGE_H_
#define SRC_TILE_CORE_DECODESTAGE_H_

#include "InstructionHandler.h"
#include "PipelineStage.h"

namespace Compute {

class DecodeStage: public MiddlePipelineStage {

//============================================================================//
// Constructors and destructors
//============================================================================//
public:
  DecodeStage(sc_module_name name);

//============================================================================//
// Methods
//============================================================================//

public:

  virtual void execute();

  // Part of the CoreInterface implemented here.
  virtual void computeLatency(opcode_t opcode, function_t fn=(function_t)0);
  virtual void readRegister(RegisterIndex index, PortIndex port);
  virtual void readPredicate();
  virtual void readCMT(RegisterIndex index);
  virtual void fetch(MemoryAddr address, ChannelMapEntry::MemoryChannel channel,
                     bool execute, bool persistent);
  virtual void jump(JumpOffset offset);
  virtual void startRemoteExecution(ChannelID address);
  virtual void endRemoteExecution();
  virtual void waitForCredit(ChannelIndex channel);
  virtual void selectChannelWithData(uint bitmask);

//============================================================================//
// Local state
//============================================================================//

private:

  // When in remote execution mode, all instructions are sent to the specified
  // network address without being executed locally. Remote execution ends when
  // the final instruction in the instruction packet has been forwarded.
  bool remoteMode;
  ChannelID remoteDestination;

  // Keep a copy of the previous instruction to help detect dependencies.
  DecodedInstruction previousInstruction;

};

} // end namespace

#endif /* SRC_TILE_CORE_DECODESTAGE_H_ */
