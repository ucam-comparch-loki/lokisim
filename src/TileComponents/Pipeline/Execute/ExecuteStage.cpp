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
  return alu.area();
}

double ExecuteStage::energy() const {
  return alu.energy();
}

bool ExecuteStage::getPredicate() const {
  return parent()->readPredReg();
}

void ExecuteStage::setPredicate(bool val) const {
  parent()->writePredReg(val);
}

void ExecuteStage::newInput(DecodedInst& operation) {

  // Receive any forwarded values if they had not been written to registers
  // early enough.
  checkForwarding(operation);

  // Execute the instruction.
  bool success = alu.execute(operation);

  // Update the contents of any forwarding paths. Should this happen every
  // cycle, or just the ones when an instruction is executed?
  if(success) {
    updateForwarding(operation);
    dataOut.write(operation);
  }

}

bool ExecuteStage::isStalled() const {
  return false; // alu.isBusy(); if/when we have multi-cycle operations
}

void ExecuteStage::checkForwarding(DecodedInst& inst) const {
  parent()->checkForwarding(inst);
}

void ExecuteStage::updateForwarding(const DecodedInst& inst) const {
  parent()->updateForwarding(inst);
}

ExecuteStage::ExecuteStage(sc_module_name name, ComponentID ID) :
    PipelineStage(name, ID),
    StageWithPredecessor(name, ID),
    StageWithSuccessor(name, ID),
    alu("alu") {

}

ExecuteStage::~ExecuteStage() {

}
