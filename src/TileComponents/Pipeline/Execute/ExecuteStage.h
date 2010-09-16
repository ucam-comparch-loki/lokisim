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
//   clock
//   stall
//   idle

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
  ExecuteStage(sc_module_name name);
  virtual ~ExecuteStage();

//==============================//
// Methods
//==============================//

public:

  virtual double area()  const;
  virtual double energy() const;

private:

  // The task performed at the beginning of each clock cycle.
  virtual void newCycle();

  // Read the predicate register.
  bool getPredicate() const;

  // Write to the predicate register.
  void setPredicate(bool val);

//==============================//
// Components
//==============================//

private:

  ALU                 alu;

  friend class ALU;

};

#endif /* EXECUTESTAGE_H_ */
