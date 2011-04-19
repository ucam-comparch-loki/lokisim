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

bool ExecuteStage::readPredicate() const {return parent()->readPredReg();}
int32_t ExecuteStage::readReg(RegisterIndex reg) const {return parent()->readReg(reg);}
int32_t ExecuteStage::readWord(MemoryAddr addr) const {return parent()->readMemWord(addr);}
int32_t ExecuteStage::readByte(MemoryAddr addr) const {return parent()->readMemByte(addr);}

void ExecuteStage::writePredicate(bool val) const {parent()->writePredReg(val);}
void ExecuteStage::writeReg(RegisterIndex reg, Word data) const {parent()->writeReg(reg, data.toInt());}
void ExecuteStage::writeWord(MemoryAddr addr, Word data) const {parent()->writeMemWord(addr, data);}
void ExecuteStage::writeByte(MemoryAddr addr, Word data) const {parent()->writeMemByte(addr, data);}

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
    alu("alu", ID) {

}

ExecuteStage::~ExecuteStage() {

}
