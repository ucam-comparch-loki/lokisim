/*
 * WriteStage.h
 *
 * Write stage of the pipeline. Responsible for sending the computation results
 * to be stored in registers, or out onto the network for another cluster or
 * memory to use.
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef WRITESTAGE_H_
#define WRITESTAGE_H_

#include "../PipelineStage.h"
#include "SendChannelEndTable.h"
#include "../../../Multiplexor/Multiplexor2.h"

class WriteStage: public PipelineStage {

//==============================//
// Ports
//==============================//

public:

  // The data from the ALU to be written or sent out onto the network.
  sc_in<Data>             fromALU;

  // The instruction to be sent to a remote cluster.
  sc_in<Instruction>      inst;

  // The NUM_SEND_CHANNELS output channels.
  sc_out<AddressedWord>  *output;

  // A flow control signal for each output (NUM_SEND_CHANNELS).
  sc_in<bool>            *flowControl;

  // The register to write data to.
  sc_in<short>            inRegAddr;

  // Tell the register file which register to write to.
  sc_out<short>           outRegAddr;

  // The indirect register to write data to.
  sc_in<short>            inIndAddr;

  // Tell the register file which indirect register to write to.
  sc_out<short>           outIndAddr;

  // The data to write to the registers.
  sc_out<Word>            regData;

  // The remote channel to send data out on.
  sc_in<short>            remoteChannel;

  // The memory operation being performed.
  sc_in<short>            memoryOp;

  // Stall the pipeline until this channel is empty.
  sc_in<short>            waitOnChannel;

  // Signal that the pipeline should stall because a send channel is full.
  sc_out<bool>            stallOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(WriteStage);
  WriteStage(sc_module_name name);
  virtual ~WriteStage();

//==============================//
// Methods
//==============================//

public:

  virtual double area()  const;
  virtual double energy() const;

private:

  virtual void  newCycle();

  void receivedInst();
  void receivedData();
  void receivedRChannel();
  void select();

  Word getMemoryRequest();

//==============================//
// Components
//==============================//

private:

  SendChannelEndTable scet;
  Multiplexor2<Word>  mux;

//==============================//
// Local state
//==============================//

private:

  // Choose which input of the multiplexor to use.
  // Only one input can be active at once.
  short selectVal;
  bool  haveNewChannel;

//==============================//
// Signals (wires)
//==============================//

private:

  sc_buffer<Word>     ALUtoRegs, muxOutput;
  sc_buffer<Word>     ALUtoMux, instToMux;
  sc_buffer<short>    muxSelect;
  sc_buffer<short>    waitChannelSig;
  sc_signal<bool>     newInstSig, newDataSig;

};

#endif /* WRITESTAGE_H_ */
