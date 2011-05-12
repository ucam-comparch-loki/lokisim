/*
 * StageWithPredecessor.cpp
 *
 *  Created on: 27 Oct 2010
 *      Author: db434
 */

#include "StageWithPredecessor.h"
#include "../../Datatype/DecodedInst.h"

void StageWithPredecessor::execute() {
  while(true) {
    // Wait for a new instruction to arrive.
    wait(dataIn.default_event());

    // Deal with the new input. We are currently not idle.
    idle.write(false);
    DecodedInst inst = dataIn.read(); // Don't want a const input.
    newInput(inst);

    // Once the next cycle starts, revert to being idle.
    wait(clock.posedge_event());
    idle.write(true);
  }
}

void StageWithPredecessor::updateReady() {
  while(true) {
    // Wait until some point late in the cycle, so we know that any operations
    // will have completed.
    wait(clock.negedge_event());

    // Write our current stall status.
    readyOut.write(!isStalled());

    if(DEBUG && isStalled() && readyOut.read()) {
      cout << this->name() << " stalled." << endl;
    }
  }
}

StageWithPredecessor::StageWithPredecessor(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID) {

  readyOut.initialize(false);

}

StageWithPredecessor::~StageWithPredecessor() {

}
