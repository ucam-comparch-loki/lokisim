/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"
#include "../../Cluster.h"
#include "../../../Utility/InstructionMap.h"

double WriteStage::area() const {
  return scet.area();// + mux.area();
}

double WriteStage::energy() const {
  return scet.energy();// + mux.energy();
}

void WriteStage::execute() {
  idle.write(true);

  // Allow any signals to propagate before starting execution.
  wait(sc_core::SC_ZERO_TIME);

  while(true) {
    // Wait for new data to arrive.
    wait(result.default_event());

    // Deal with the new input. We are currently not idle.
    idle.write(false);
    newInput();

    // Once the next cycle starts, revert to being idle.
    wait(clock.posedge_event());
    idle.write(true);
  }
}

void WriteStage::newInput() {
  DecodedInst dec = result.read();

  if(DEBUG) cout << this->name() << " received Data: " << dec.result()
                 << endl;

  // Put data into the send channel-end table.
  scet.write(dec);

  // Write to registers (they ignore the write if the index is invalid).
  writeReg(dec.destinationReg(), dec.result(),
             dec.operation() == InstructionMap::IWTR);
}

void WriteStage::updateReady() {
  readyOut.write(true);

  while(true) {
    wait(clock.negedge_event());
    scet.send();
    readyOut.write(!scet.isFull());
  }
}

void WriteStage::writeReg(RegisterIndex reg, int32_t value, bool indirect) {
  parent()->writeReg(reg, value, indirect);
}

WriteStage::WriteStage(sc_module_name name, ComponentID ID) :
    PipelineStage(name),
    scet("scet", ID) {

  id = ID;

  output      = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];
  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];

  // Connect the SCET to the network.
  for(uint i=0; i<NUM_SEND_CHANNELS; i++) {
    scet.output[i](output[i]);
    scet.flowControl[i](flowControl[i]);
  }

  SC_THREAD(updateReady);

}

WriteStage::~WriteStage() {
  delete[] output;
  delete[] flowControl;
}
