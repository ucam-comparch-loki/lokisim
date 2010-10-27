/*
 * ExecuteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "ExecuteStage.h"
#include "../../Cluster.h"
#include "../../../Datatype/DecodedInst.h"

double ExecuteStage::area() const {
  return alu.area();// + in1Mux.area() + in2Mux.area();
}

double ExecuteStage::energy() const {
  return alu.energy();// + in1Mux.energy() + in2Mux.energy();
}

bool ExecuteStage::getPredicate() const {
  return parent()->readPredReg();
}

void ExecuteStage::setPredicate(bool val) {
  parent()->writePredReg(val);
}

void ExecuteStage::newInput() {

  DecodedInst op = operation.read();

  // Receive any forwarded values if they had not been written to registers
  // early enough.
  checkForwarding(op);

  // Execute the instruction.
  bool success = alu.execute(op);

  // Update the contents of any forwarding paths. Should this happen every
  // cycle, or just the ones when an instruction is executed?
  if(success) {
    updateForwarding(op);
    result.write(op);
  }

}

void ExecuteStage::execute() {
  idle.write(true);

  // Allow any signals to propagate before starting execution.
  wait(sc_core::SC_ZERO_TIME);

  // We then loop forever, executing the appropriate tasks each clock cycle.
  while(true) {
    // Wait until we receive input.
    wait(operation.default_event());

    // Operate on the input. We are currently not idle.
    idle.write(false);
    newInput();

    // Once the next cycle starts, revert to being idle.
    wait(clock.posedge_event());
    idle.write(true);
  }
}

void ExecuteStage::checkForwarding(DecodedInst& inst) {
  // We don't want to use forwarded data if we read from register 0: this could
  // mean that there is just no register specified, and we are using an
  // immediate instead.
  if(inst.sourceReg1() > 0) {
    if(inst.sourceReg1() == previousDest1) inst.operand1(previousResult1);
    else if(inst.sourceReg1() == previousDest2) inst.operand1(previousResult2);
  }
  if(inst.sourceReg2() > 0) {
    if(inst.sourceReg2() == previousDest1) inst.operand2(previousResult1);
    else if(inst.sourceReg2() == previousDest2) inst.operand2(previousResult2);
  }
}

void ExecuteStage::updateForwarding(DecodedInst& inst) {
  previousDest2   = previousDest1;
  previousResult2 = previousResult1;

  // We don't want to forward any data which was sent to register 0, because
  // r0 doesn't store values: it is a constant.
  previousDest1   = (inst.destinationReg() == 0) ? -1 : inst.destinationReg();
  previousResult1 = inst.result();
}

void ExecuteStage::updateReady() {
  readyOut.write(true);

  while(true) {
    wait(clock.negedge_event());
    readyOut.write(readyIn.read()); //&& !alu.busy();
  }
}

ExecuteStage::ExecuteStage(sc_module_name name, ComponentID ID) :
    PipelineStage(name),
    alu("alu") {

  id = ID;
  previousDest1 = previousDest2 = -1;

  SC_THREAD(updateReady);

}

ExecuteStage::~ExecuteStage() {

}
