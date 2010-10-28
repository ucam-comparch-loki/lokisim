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
  return scet.area();
}

double WriteStage::energy() const {
  return scet.energy();
}

void WriteStage::newInput(DecodedInst& data) {
  if(DEBUG) cout << this->name() << " received Data: " << data.result()
                 << endl;

  // Put data into the send channel-end table.
  scet.write(data);

  // Write to registers (they ignore the write if the index is invalid).
  writeReg(data.destinationReg(), data.result(),
             data.operation() == InstructionMap::IWTR);
}

bool WriteStage::isStalled() const {
  return scet.isFull();
}

void WriteStage::sendOutputs() {
  scet.send();
}

void WriteStage::writeReg(RegisterIndex reg, int32_t value, bool indirect) {
  parent()->writeReg(reg, value, indirect);
}

WriteStage::WriteStage(sc_module_name name, ComponentID ID) :
    PipelineStage(name),
    StageWithPredecessor(name),
    scet("scet", ID) {

  id = ID;

  output      = new sc_out<AddressedWord>[NUM_SEND_CHANNELS];
  flowControl = new sc_in<bool>[NUM_SEND_CHANNELS];

  // Connect the SCET to the network.
  for(uint i=0; i<NUM_SEND_CHANNELS; i++) {
    scet.output[i](output[i]);
    scet.flowControl[i](flowControl[i]);
  }

}

WriteStage::~WriteStage() {
  delete[] output;
  delete[] flowControl;
}
