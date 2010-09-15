/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"
#include "../../Cluster.h"

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

        if(DEBUG) cout << this->name() << " received Data: " << dec.getResult()
                       << endl;

        // Put data into the send channel-end table.
        scet.write(dec);

        // Write to registers (they ignore the write if the index is invalid).
        writeReg(dec.getDestination(), dec.getResult(),
                   dec.getOperation() == InstructionMap::IWTR);
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
