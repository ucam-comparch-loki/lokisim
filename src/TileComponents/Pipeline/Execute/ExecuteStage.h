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

//==============================//
// Ports
//==============================//

public:

  // Two data inputs from the receive channel-end table.
  sc_in<Data>         fromRChan1, fromRChan2;

  // Two data inputs from the register file.
  sc_in<Data>         fromReg1, fromReg2;

  // Two data inputs forwarded from the result of the previous operation.
  sc_in<Data>         fromALU1, fromALU2;

  // Data input from the sign extender.
  sc_in<Data>         fromSExtend;

  // Result of this computation.
  sc_out<Data>        output;

  // The operation to carry out.
  sc_in<short>        operation;

  // Select where to get both inputs of the computation from.
  sc_in<short>        op1Select, op2Select;

  // Tells whether the result of this computation should be written to the
  // predicate register.
  sc_in<bool>         setPredicate;

  // The predicate value being written to the register.
  sc_out<bool>        predicate;

  // The register to write the result of this computation to (passing through).
  sc_in<short>        writeIn;
  sc_out<short>       writeOut;

  // The indirect register to write the result to (passing through).
  sc_in<short>        indWriteIn;
  sc_out<short>       indWriteOut;

  // The remote channel to send the result to (passing through).
  sc_in<short>        remoteChannelIn;
  sc_out<short>       remoteChannelOut;

  // The memory operation being performed. This may be different to the
  // ALU operation because, for example, a load may require an addition to
  // compute the address. (Passing through).
  sc_in<short>        memoryOpIn;
  sc_out<short>       memoryOpOut;

  // Stall the pipeline until this output channel is empty.
  sc_in<short>        waitOnChannelIn;
  sc_out<short>       waitOnChannelOut;

  // The instruction to send to a remote cluster (passing through).
  sc_in<Instruction>  remoteInstIn;
  sc_out<Instruction> remoteInstOut;

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

  virtual void newCycle();

//==============================//
// Components
//==============================//

private:

  ALU                 alu;
  Multiplexor3<Data>  in1Mux;
  Multiplexor4<Data>  in2Mux;

//==============================//
// Signals (wires)
//==============================//

private:

  sc_signal<Data>     toALU1, toALU2, outputSig;
  sc_buffer<short>    in1Select, in2Select, ALUSelect;
  sc_signal<bool>     setPredSig;

};

#endif /* EXECUTESTAGE_H_ */
