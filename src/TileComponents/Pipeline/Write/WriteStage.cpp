/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"
#include "../../../Datatype/MemoryRequest.h"

double WriteStage::area() const {
  return scet.area() + mux.area();
}

double WriteStage::energy() const {
  return scet.energy() + mux.energy();
}

void WriteStage::newCycle() {

  while(true) {
    if(!stall.read()) {
      // There are three possible signals showing activity, so we need an
      // uglier way to test whether any of them are doing anything.
      bool active = false;

//      if(fromALU.event()) {
//        regData.write(fromALU.read());
//        active = true;
//      }
//
//      if(inRegAddr.event()) {
//        outRegAddr.write(inRegAddr.read());
//        active = true;
//      }
//
//      if(inIndAddr.event()) {
//        outIndAddr.write(inIndAddr.read());
//        active = true;
//      }

      idle.write(!active);

//      COPY_IF_NEW(waitOnChannel, waitChannelSig);
    }

    wait(clock.posedge_event());
  }

}

/* Change the multiplexor's select signal so it uses the new Instruction */
void WriteStage::receivedInst() {
//  instToMux.write(inst.read());
  selectVal = 1;   // Want this Instruction to get into the SCET
  newInstSig.write(!newInstSig.read());

//  if(DEBUG) cout<<this->name()<<" received Instruction: "<<inst.read()<<endl;
}

/* Change the multiplexor's select signal so it uses the new Data */
void WriteStage::receivedData() {

  // Generate a memory request using the new data, if necessary.
//  if(memoryOp.event()) ALUtoMux.write(getMemoryRequest());
//  else ALUtoMux.write(fromALU.read());

  selectVal = 0;   // Want this Data to get into the SCET
  newDataSig.write(!newDataSig.read());

//  if(DEBUG) cout<< this->name() << " received Data: " << fromALU.read() <<endl;
}

void WriteStage::select() {
  // Only write the select value if we have received a new valid channel ID.
  // If we don't write the select value, the data doesn't reach the SCET.
//  if(remoteChannel.event() && remoteChannel.read() != Instruction::NO_CHANNEL) {
//    muxSelect.write(selectVal);
//  }
}

/* Generate a memory request using the address from the ALU and the operation
 * supplied by the decoder. */
Word WriteStage::getMemoryRequest() const {
  MemoryRequest mr;//(fromALU.read().getData(), memoryOp.read());
  return mr;
}

WriteStage::WriteStage(sc_module_name name) :
    PipelineStage(name),
    scet("scet"),
    mux("writemux") {

  output      = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];
  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];

// Register methods
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
//  scet.remoteChannel(remoteChannel);
  scet.waitOnChannel(waitChannelSig);

  for(int i=0; i<NUM_SEND_CHANNELS; i++) {
    scet.output[i](output[i]);
    scet.flowControl[i](flowControl[i]);
  }

}

WriteStage::~WriteStage() {

}
