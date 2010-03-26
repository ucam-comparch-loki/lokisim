/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"
#include "../../../Datatype/MemoryRequest.h"

void WriteStage::newCycle() {

  while(true) {
    if(!stall.read()) {
      COPY_IF_NEW(fromALU, regData);
      COPY_IF_NEW(inRegAddr, outRegAddr);
      COPY_IF_NEW(inIndAddr, outIndAddr);
      COPY_IF_NEW(waitOnChannel, waitChannelSig);
    }

    wait(clock.posedge_event());
  }

}

/* Change the multiplexor's select signal so it uses the new Instruction */
void WriteStage::receivedInst() {
  instToMux.write(inst.read());
  selectVal = 1;   // Want this Instruction to get into the SCET
  newInstSig.write(!newInstSig.read());

  if(DEBUG) cout << "WriteStage received Instruction: " << inst.read() << endl;
}

/* Change the multiplexor's select signal so it uses the new Data */
void WriteStage::receivedData() {

  // Generate a memory request using the new data, if necessary.
  if(memoryOp.event()) ALUtoMux.write(getMemoryRequest());
  else ALUtoMux.write(fromALU.read());

  selectVal = 0;   // Want this Data to get into the SCET
  newDataSig.write(!newDataSig.read());

  if(DEBUG) cout << "WriteStage received Data: " << fromALU.read() << endl;
}

void WriteStage::select() {
  if(remoteChannel.event()) muxSelect.write(selectVal);
}

/* Generate a memory request using the address from the ALU and the operation
 * supplied by the decoder. */
Word WriteStage::getMemoryRequest() {
  MemoryRequest mr(fromALU.read().getData(), memoryOp.read());
  return mr;
}

WriteStage::WriteStage(sc_module_name name) :
    PipelineStage(name),
    scet("scet"),
    mux("writemux") {

  output      = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];
  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];

// Register methods
  SC_METHOD(receivedInst);
  sensitive << inst;
  dont_initialize();

  SC_METHOD(receivedData);
  sensitive << fromALU;
  dont_initialize();

  SC_METHOD(select);
  sensitive << newInstSig << newDataSig;
  dont_initialize();

// Connect everything up
  mux.in1(ALUtoMux);
  mux.in2(instToMux);
  mux.select(muxSelect);
  mux.result(muxOutput); scet.input(muxOutput);

  scet.clock(clock);
  scet.stallOut(stallOut);
  scet.remoteChannel(remoteChannel);
  scet.waitOnChannel(waitChannelSig);

  for(int i=0; i<NUM_SEND_CHANNELS; i++) {
    scet.output[i](output[i]);
    scet.flowControl[i](flowControl[i]);
  }

}

WriteStage::~WriteStage() {

}
