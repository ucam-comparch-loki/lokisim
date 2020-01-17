/*
 * ExecuteStage.h
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_EXECUTESTAGE_H_
#define SRC_TILE_CORE_EXECUTESTAGE_H_

#include "InstructionHandler.h"
#include "PipelineStage.h"

namespace Compute {

class ExecuteStage: public MiddlePipelineStage {

//============================================================================//
// Constructors and destructors
//============================================================================//
public:
  ExecuteStage(sc_module_name name);

//============================================================================//
// Methods
//============================================================================//

public:

  virtual void execute();

  // Part of the CoreInterface implemented here.
  virtual void computeLatency(opcode_t opcode, function_t fn=(function_t)0);
  virtual void readCMT(RegisterIndex index);
  virtual void writeCMT(RegisterIndex index, EncodedCMTEntry value);
  virtual void readCreg(RegisterIndex index);
  virtual void writeCreg(RegisterIndex index, int32_t value);
  virtual void readScratchpad(RegisterIndex index);
  virtual void writeScratchpad(RegisterIndex index, int32_t value);
  virtual void writePredicate(bool value);
  virtual void syscall(int code);

//============================================================================//
// Local state
//============================================================================//

private:
  friend class ComputeHandler;
  friend class ReadCMTHandler;
  friend class WriteCMTHandler;
  friend class ReadCRegsHandler;
  friend class WriteCRegsHandler;
  friend class ReadScratchpadHandler;
  friend class WriteScratchpadHandler;
  friend class WritePredicateHandler;

  ComputeHandler computeHandler;
  ReadCMTHandler readCMTHandler;
  WriteCMTHandler writeCMTHandler;
  ReadCRegsHandler readCregsHandler;
  WriteCRegsHandler writeCregsHandler;
  ReadScratchpadHandler readScratchpadHandler;
  WriteScratchpadHandler writeScratchpadHandler;
  WritePredicateHandler writePredicateHandler;

};

} // end namespace

#endif /* SRC_TILE_CORE_EXECUTESTAGE_H_ */
