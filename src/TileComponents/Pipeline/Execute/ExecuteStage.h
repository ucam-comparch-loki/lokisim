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
//   sc_in<bool>  clock
//   sc_out<bool> idle

  // The input operation to perform.
  sc_in<DecodedInst>  operation;

  // The output result of the operation.
  sc_out<DecodedInst> result;

  // Tells whether the write stage is ready to receive new data.
  sc_in<bool>         readyIn;

  // Tell the decode stage whether we are ready to receive a new operation.
  sc_out<bool>        readyOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(ExecuteStage);
  ExecuteStage(sc_module_name name, ComponentID ID);
  virtual ~ExecuteStage();

//==============================//
// Methods
//==============================//

public:

  virtual double area()  const;
  virtual double energy() const;

private:

  // The task performed when a new operation is received.
  virtual void newInput();

  virtual void execute();

  // Read the predicate register.
  bool getPredicate() const;

  // Write to the predicate register.
  void setPredicate(bool val);

  // Check the forwarding paths to see if this instruction is expecting a value
  // which hasn't been written to registers yet.
  void checkForwarding(DecodedInst& inst);

  // Update the forwarding paths using this recently-executed instruction.
  void updateForwarding(DecodedInst& inst);

  // Update our output ready signal. Executes once per cycle.
  void updateReady();

//==============================//
// Components
//==============================//

private:

  ALU                 alu;

  friend class ALU;

//==============================//
// Local state
//==============================//

private:

  // Store the previous two results and destination registers, to allow data
  // forwarding when necessary. 2 is older than 1.
  uint8_t previousDest1, previousDest2;
  int64_t previousResult1, previousResult2;

};

#endif /* EXECUTESTAGE_H_ */
