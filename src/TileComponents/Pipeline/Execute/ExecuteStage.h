/*
 * ExecuteStage.h
 *
 * Execute stage of the pipeline. Responsible for transforming the given
 * data and outputting the result.
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef EXECUTESTAGE_H_
#define EXECUTESTAGE_H_

#include "../PipelineStage.h"
#include "ALU.h"

class ExecuteStage: public PipelineStage {

//==============================//
// Ports
//==============================//

public:

// Inherited from PipelineStage:
//   sc_in<bool>          clock
//   sc_out<bool>         idle
//   sc_out<bool>         stallOut

  // The input instruction to be working on. DecodedInst holds all information
  // required for any pipeline stage to do its work.
  sc_in<DecodedInst>  dataIn;

  // Tell whether this stage is ready for input (ignoring effects of any other stages).
  sc_out<bool>        readyOut;

  // The decoded instruction after passing through this pipeline stage.
  // DecodedInst holds all necessary fields for data at all stages throughout
  // the pipeline.
  sc_out<DecodedInst> dataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(ExecuteStage);
  ExecuteStage(sc_module_name name, const ComponentID& ID);

//==============================//
// Methods
//==============================//

private:

  // The main loop controlling this stage. Involves waiting for new input,
  // doing work on it, and sending it to the next stage.
  virtual void execute();

  // Determine whether this stage is stalled or not, and write the appropriate
  // output.
  virtual void updateReady();

  // The task performed when a new operation is received.
  virtual void newInput(DecodedInst& operation);

  virtual bool isStalled() const;

  // Read the predicate register.
  bool readPredicate() const;
  int32_t readReg(RegisterIndex reg) const;
  int32_t readWord(MemoryAddr addr) const;
  int32_t readByte(MemoryAddr addr) const;

  // Write to the predicate register.
  void writePredicate(bool val) const;
  void writeReg(RegisterIndex reg, Word data) const;
  void writeWord(MemoryAddr addr, Word data) const;
  void writeByte(MemoryAddr addr, Word data) const;

  // Check the forwarding paths to see if this instruction is expecting a value
  // which hasn't been written to registers yet.
  void checkForwarding(DecodedInst& inst) const;

  // Update the forwarding paths using this recently-executed instruction.
  void updateForwarding(const DecodedInst& inst) const;

//==============================//
// Components
//==============================//

private:

  ALU alu;

  friend class ALU;

//==============================//
// Local state
//==============================//

  sc_core::sc_event executedInstruction;

};

#endif /* EXECUTESTAGE_H_ */
