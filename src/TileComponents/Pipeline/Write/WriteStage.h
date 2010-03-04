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
#include "../../../Multiplexor/Multiplexor2.h"
#include "SendChannelEndTable.h"
#include "../../../Datatype/Array.h"

class WriteStage: public PipelineStage {

public:
/* Ports */
  sc_in<Data>             fromALU;
  sc_in<Instruction>      inst;
  sc_in<bool>            *flowControl;  // array
  sc_in<short>            inRegAddr, inIndAddr, remoteChannel;
  sc_out<short>           outRegAddr, outIndAddr;
  sc_out<Word>            regData;
  sc_out<AddressedWord>  *output;       // array

/* Constructors and destructors */
  SC_HAS_PROCESS(WriteStage);
  WriteStage(sc_module_name name);
  virtual ~WriteStage();

private:
/* Methods */
  virtual void  newCycle();

  void receivedInst();
  void receivedData();
  void receivedRChannel();
  void select();

/* Components */
  SendChannelEndTable scet;
  Multiplexor2<Word>  mux;

/* Local state */
  // Choose which input of the multiplexor to use.
  // Only one input can be active at once.
  short selectVal;
  bool  haveNewChannel;

/* Signals (wires) */
  sc_buffer<Word>     ALUtoRegs, muxOutput;
  sc_buffer<Word>     ALUtoMux, instToMux;
  sc_buffer<short>    muxSelect;
  sc_signal<bool>     newInstSig, newDataSig;

};

#endif /* WRITESTAGE_H_ */
