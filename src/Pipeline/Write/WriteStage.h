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
#include "../../Multiplexor/Multiplexor2.h"
#include "SendChannelEndTable.h"
#include "../../Datatype/Array.h"

class WriteStage: public PipelineStage {

/* Components */
  SendChannelEndTable scet;
  Multiplexor2<Word> mux;

/* Local state */
  // Choose which input of the multiplexor to use.
  // Only one input can be active at once.
  short select;

/* Signals (wires) */
  sc_signal<Word> ALUtoRegs, ALUtoMux, instToMux, muxOutput;
  sc_signal<Array<AddressedWord> > out;
  sc_signal<short> muxSelect;

/* Methods */
  virtual void newCycle();

  void receivedInst();
  void receivedData();

public:
/* Ports */
  sc_in<Data> fromALU;
  sc_in<Instruction> inst;
  sc_in<short> inRegAddr, inIndAddr;
  sc_out<short> outRegAddr, outIndAddr;
  sc_out<Word> regData;
  sc_out<Array<AddressedWord> > output;

/* Constructors and destructors */
  SC_HAS_PROCESS(WriteStage);
  WriteStage(sc_core::sc_module_name name, int ID);
  virtual ~WriteStage();
};

#endif /* WRITESTAGE_H_ */
