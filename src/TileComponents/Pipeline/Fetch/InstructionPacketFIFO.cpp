/*
 * InstructionPacketFIFO.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketFIFO.h"

Instruction InstructionPacketFIFO::read() {
  Instruction inst = fifo.read();
  updateFlowControl();
  return inst;
}

void InstructionPacketFIFO::write(Instruction inst) {
  fifo.write(inst);
  updateFlowControl();
  if(DEBUG) cout << this->name() << " received Instruction:  " << inst << endl;
}

void InstructionPacketFIFO::updateFlowControl() {
  flowControl.write(fifo.remainingSpace());
}

bool InstructionPacketFIFO::isEmpty() {
  return fifo.isEmpty();
}

InstructionPacketFIFO::InstructionPacketFIFO(sc_module_name name) :
    Component(name),
    fifo(IPK_FIFO_SIZE) {

}

InstructionPacketFIFO::~InstructionPacketFIFO() {

}
