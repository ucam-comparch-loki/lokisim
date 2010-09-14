/*
 * ExecuteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "ExecuteStage.h"

double ExecuteStage::area() const {
  return alu.area();// + in1Mux.area() + in2Mux.area();
}

double ExecuteStage::energy() const {
  return alu.energy();// + in1Mux.energy() + in2Mux.energy();
}

/* Simulate pipelining by only allowing signals through at the start of a cycle */
void ExecuteStage::newCycle() {
  while(true) {
    if(!readyIn.read()) {
      readyOut.write(false);
    }
    else if(!stall.read()) {
      if(operation.event()) {
        DecodedInst op = operation.read();
        bool success = alu.execute(op);

        if(success) result.write(op);

        readyOut.write(true); // readyOut.write(alu.busy())?
        idle.write(false);
      }
      else {
        idle.write(true);
        readyOut.write(true);
      }
    }
    wait(clock.posedge_event());
  }
}

ExecuteStage::ExecuteStage(sc_module_name name) :
    PipelineStage(name),
    alu("alu") {

}

ExecuteStage::~ExecuteStage() {

}
