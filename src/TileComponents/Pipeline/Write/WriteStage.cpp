/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"
#include "../../Cluster.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Utility/InstructionMap.h"

double WriteStage::area() const {
  return scet.area();
}

double WriteStage::energy() const {
  return scet.energy();
}

void WriteStage::execute() {
  while(true) {
    // Wait for a new instruction to arrive.
    wait(dataIn.default_event() | fromFetchLogic.default_event());

    // Deal with the new input. We are currently not idle.
    idle.write(false);

    // Enter a loop in case we receive data from both inputs at once.
    while(true) {
      if(dataIn.event()) {
        DecodedInst inst = dataIn.read(); // Don't want a const input.
        newInput(inst);
      }
      else if(fromFetchLogic.event()) {
        scet.write(fromFetchLogic.read(), 0);
      }
      else break;

      wait(clock.posedge_event());
    }

    idle.write(true);
  }
}

void WriteStage::newInput(DecodedInst& data) {
  if(DEBUG) cout << this->name() << " received Data: " << data.result()
                 << endl;

  // Put data into the send channel-end table.
  scet.write(data);

  // Write to registers (they ignore the write if the index is invalid).
  if(InstructionMap::storesResult(data.operation())) {
    writeReg(data.destination(), data.result(),
             data.operation() == InstructionMap::IWTR);
  }

  // Do we need to say we are stalling because of output if the SCET is full?
}

bool WriteStage::isStalled() const {
  return scet.full();
}

void WriteStage::sendOutputs() {
  scet.send();
}

void WriteStage::writeReg(RegisterIndex reg, int32_t value, bool indirect) const {
  parent()->writeReg(reg, value, indirect);
}

WriteStage::WriteStage(sc_module_name name, ComponentID ID) :
    PipelineStage(name, ID),
    StageWithPredecessor(name, ID),
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
