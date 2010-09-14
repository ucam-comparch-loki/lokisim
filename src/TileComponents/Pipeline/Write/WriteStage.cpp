/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"
#include "../../Cluster.h"
#include "../../../Datatype/MemoryRequest.h"

double WriteStage::area() const {
  return scet.area();// + mux.area();
}

double WriteStage::energy() const {
  return scet.energy();// + mux.energy();
}

void WriteStage::newCycle() {

  while(true) {
    if(!stall.read()) {

      bool active = result.event();

      if(active) {
        DecodedInst dec = result.read();

        // Put data into the send channel-end table.
        scet.write(dec);

        // Write to registers if there is a valid destination register.
        if(dec.getDestination() != 0) {
          writeReg(dec.getDestination(), dec.getResult(),
                   dec.getOperation() == InstructionMap::IWTR);
        }
      }

      idle.write(!active);
    }

    // Attempt to send data onto the network if possible.
    scet.send();
    readyOut.write(!scet.isFull());

    wait(clock.posedge_event());
  }

}

void WriteStage::writeReg(uint8_t reg, int32_t value, bool indirect) {
  parent()->writeReg(reg, value, indirect);
}

/* Change the multiplexer's select signal so it uses the new Data */
//void WriteStage::receivedData() {
//
//  // Generate a memory request using the new data, if necessary.
////  if(memoryOp.event()) ALUtoMux.write(getMemoryRequest());
////  else ALUtoMux.write(fromALU.read());
//
//  selectVal = 0;   // Want this Data to get into the SCET
//  newDataSig.write(!newDataSig.read());
//
////  if(DEBUG) cout<< this->name() << " received Data: " << fromALU.read() <<endl;
//}

/* Generate a memory request using the address from the ALU and the operation
 * supplied by the decoder. */
Word WriteStage::getMemoryRequest() const {
  MemoryRequest mr;//(fromALU.read().getData(), memoryOp.read());
  return mr;
}

WriteStage::WriteStage(sc_module_name name) :
    PipelineStage(name),
    scet("scet") {

  output      = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];
  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];

  // Connect the SCET to the network.
  for(int i=0; i<NUM_SEND_CHANNELS; i++) {
    scet.output[i](output[i]);
    scet.flowControl[i](flowControl[i]);
  }

}

WriteStage::~WriteStage() {

}
