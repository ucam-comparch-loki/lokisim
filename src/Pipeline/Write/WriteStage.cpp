/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"

void WriteStage::newCycle() {
  Word w = static_cast<Word>(fromALU.read());

  ALUtoRegs.write(w);
  outRegAddr.write(inRegAddr.read());
  outIndAddr.write(inIndAddr.read());

  muxSelect.write(select);
}

/* Cast the input Instruction to a Word when we receive it */
void WriteStage::receivedInst() {
  Word w = static_cast<Word>(inst.read());
  instToMux.write(w);
  select = 1;   // Want this Instruction to get into the SCET
}

/* Cast the input Data to a Word when we receive it */
void WriteStage::receivedData() {
  Word w = static_cast<Word>(fromALU.read());
  ALUtoMux.write(w);
  select = 0;   // Want this Data to get into the SCET
}

WriteStage::WriteStage(sc_core::sc_module_name name, int ID) :
    PipelineStage(name, ID),
    scet("scet", ID),
    mux("writemux") {

// Register methods
  SC_METHOD(receivedInst);
  sensitive << inst;

  SC_METHOD(receivedData);
  sensitive << fromALU;

// Connect everything up
  regData(ALUtoRegs);
  mux.in1(ALUtoMux);
  mux.in2(instToMux);
  mux.select(muxSelect);
  mux.result(muxOutput); scet.input(muxOutput);

  scet.output(output);

}

WriteStage::~WriteStage() {

}
