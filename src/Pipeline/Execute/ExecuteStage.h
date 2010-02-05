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
#include "../../Multiplexor/Multiplexor3.h"
#include "../../Multiplexor/Multiplexor4.h"
#include "ALU.h"

class ExecuteStage: public PipelineStage {

/* Components */
  ALU alu;
  Multiplexor3<Data> in1Mux;
  Multiplexor4<Data> in2Mux;

/* Signals (wires) */
  sc_signal<Data> toALU1, toALU2, outputSig;
  sc_signal<Data> rcet1, rcet2, reg1, reg2, sExtend, ALU1, ALU2;
  sc_signal<short> in1Select, in2Select, ALUSelect;

/* Methods */
  virtual void newCycle();

public:
/* Ports */
  sc_in<Data> fromRChan1, fromRChan2, fromReg1, fromReg2, fromSExtend, fromALU1, fromALU2;
  sc_out<Data> output;
  sc_in<short> op1Select, op2Select, operation;

  // Signals just passing through
  sc_in<short> writeIn, indWriteIn;
  sc_out<short> writeOut, indWriteOut;
  sc_in<Instruction> remoteInstIn;
  sc_out<Instruction> remoteInstOut;

/* Constructors and destructors */
  SC_HAS_PROCESS(ExecuteStage);
  ExecuteStage(sc_core::sc_module_name name, int ID);
  virtual ~ExecuteStage();
};

#endif /* EXECUTESTAGE_H_ */
