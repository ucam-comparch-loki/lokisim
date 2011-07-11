/*
 * ExecuteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "ExecuteStage.h"
#include "../../Cluster.h"
#include "../../../Datatype/DecodedInst.h"

bool ExecuteStage::readPredicate() const {return parent()->readPredReg();}
int32_t ExecuteStage::readReg(RegisterIndex reg) const {return parent()->readReg(reg);}
int32_t ExecuteStage::readWord(MemoryAddr addr) const {return parent()->readWord(addr).toInt();}
int32_t ExecuteStage::readByte(MemoryAddr addr) const {return parent()->readByte(addr).toInt();}

void ExecuteStage::writePredicate(bool val) const {parent()->writePredReg(val);}
void ExecuteStage::writeReg(RegisterIndex reg, Word data) const {parent()->writeReg(reg, data.toInt());}
void ExecuteStage::writeWord(MemoryAddr addr, Word data) const {parent()->writeWord(addr, data);}
void ExecuteStage::writeByte(MemoryAddr addr, Word data) const {parent()->writeByte(addr, data);}

void ExecuteStage::execute() {
  if(dataIn.event()) {
    // Deal with the new input. We are currently not idle.
    idle.write(false);
    DecodedInst inst = dataIn.read(); // Don't want a const input.
    newInput(inst);

    next_trigger(clock.posedge_event());
  }
  else {
    // There is no instruction to execute - wait for one.
    idle.write(true);
    next_trigger(dataIn.default_event());
  }
}

void ExecuteStage::updateReady() {
  bool ready = !isStalled();

  // Write our current stall status.
  if(ready != readyOut.read()) readyOut.write(ready);

  if(DEBUG && isStalled() && readyOut.read()) {
    cout << this->name() << " stalled." << endl;
  }

  // Wait until some point late in the cycle, so we know that any operations
  // will have completed.
  next_trigger(executedInstruction);
}

void ExecuteStage::newInput(DecodedInst& operation) {

  // Receive any forwarded values if they had not been written to registers
  // early enough.
  checkForwarding(operation);

  // Execute the instruction.
  bool success = alu.execute(operation);
  executedInstruction.notify();

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

ExecuteStage::ExecuteStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    alu("alu", ID) {

  SC_METHOD(execute);
  // do initialise

}
