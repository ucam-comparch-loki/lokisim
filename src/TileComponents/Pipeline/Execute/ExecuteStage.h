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

  sc_in<DecodedInst>  operation;

  sc_out<DecodedInst> result;

  sc_in<bool>         readyIn;

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

  bool getPredicate() const;
  void setPredicate(bool val);

private:

  // The task performed at the beginning of each clock cycle.
  virtual void newCycle();

//==============================//
// Components
//==============================//

private:

  ALU                 alu;

//==============================//
// Signals (wires)
//==============================//

private:

  sc_signal<Data>     toALU1, toALU2, outputSig;
  sc_buffer<short>    in1Select, in2Select, ALUSelect, usePredSig;
  sc_signal<bool>     setPredSig;

};

#endif /* EXECUTESTAGE_H_ */
