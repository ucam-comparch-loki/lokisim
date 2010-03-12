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
#include "../../../Multiplexor/Multiplexor3.h"
#include "../../../Multiplexor/Multiplexor4.h"
#include "ALU.h"

class ExecuteStage: public PipelineStage {

public:
/* Ports */
  sc_in<Data>         fromRChan1, fromReg1, fromALU1;
  sc_in<Data>         fromRChan2, fromReg2, fromALU2, fromSExtend;
  sc_out<Data>        output;
  sc_in<short>        op1Select, op2Select, operation, predicate;
  sc_in<bool>         setPredicate;

  // Signals just passing through
  sc_in<short>        writeIn, indWriteIn, remoteChannelIn;
  sc_out<short>       writeOut, indWriteOut, remoteChannelOut;
  sc_in<Instruction>  remoteInstIn;
  sc_out<Instruction> remoteInstOut;

/* Constructors and destructors */
  SC_HAS_PROCESS(ExecuteStage);
  ExecuteStage(sc_module_name name);
  virtual ~ExecuteStage();

private:
/* Methods */
  virtual void newCycle();

/* Components */
  ALU                 alu;
  Multiplexor3<Data>  in1Mux;
  Multiplexor4<Data>  in2Mux;

/* Signals (wires) */
  sc_signal<Data>     toALU1, toALU2, outputSig;
  sc_buffer<short>    in1Select, in2Select, ALUSelect;
  sc_signal<short>    predicateSig;

};

#endif /* EXECUTESTAGE_H_ */
