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
    // Wait until late in the cycle so we know that other tasks have completed.
    wait(clock.negedge_event());

    // Send any waiting outputs (this may clear space in buffers and unstall us).
    // TODO: only call this method if we know there is something to send.
    sendOutputs();

    if(DEBUG && isStalled() && readyOut.read()) {
      cout << this->name() << " stalled." << endl;
    }

    // Write our current stall status.
    readyOut.write(!isStalled());
  }
}

StageWithPredecessor::StageWithPredecessor(sc_module_name name, ComponentID ID) :
    PipelineStage(name, ID) {

  readyOut.initialize(false);

}

StageWithPredecessor::~StageWithPredecessor() {

}
