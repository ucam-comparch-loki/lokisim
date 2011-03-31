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
    // Wait for new data to arrive.
    wait(dataIn.default_event() | fromFetchLogic.default_event());

    // Whilst dealing with the new input, we are not idle.
    idle.write(false);

    // Enter a loop in case we receive data from both inputs at once.
    while(true) {
      if(!isStalled()) {
        if(dataIn.event()) {
//          cout << "data from execute" << endl;
          DecodedInst inst = dataIn.read(); // Don't want a const input.
          newInput(inst);
        }
        else if(fromFetchLogic.event()) {
//          cout << "data from decode" << endl;
          scet.write(fromFetchLogic.read(), 0);
        }
        else break;
      }

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

void WriteStage::writeReg(RegisterIndex reg, int32_t value, bool indirect) const {
  parent()->writeReg(reg, value, indirect);
}

WriteStage::WriteStage(sc_module_name name, ComponentID ID) :
    PipelineStage(name, ID),
    StageWithPredecessor(name, ID),
    scet("scet", ID) {

  id = ID;

  // Connect the SCET to the network.
  scet.clock(clock);
  scet.output(output);
  scet.network(network);
  scet.flowControl(flowControl);
  scet.creditsIn(creditsIn);

}
