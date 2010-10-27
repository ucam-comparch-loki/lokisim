/*
 * InstructionPacketFIFO.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketFIFO.h"
#include "../../../Datatype/Instruction.h"

Instruction InstructionPacketFIFO::read() {
  Instruction inst = fifo.read();
  sendCredit();
  return inst;
}

void InstructionPacketFIFO::write(Instruction inst) {
  fifo.write(inst);
  if(DEBUG) cout << this->name() << " received Instruction:  " << inst << endl;
}

void InstructionPacketFIFO::sendCredit() {
  flowControl.write(1);
}

bool InstructionPacketFIFO::isEmpty() {
  return fifo.isEmpty();
}

InstructionPacketFIFO::InstructionPacketFIFO(sc_module_name name) :
    Component(name),
    fifo(IPK_FIFO_SIZE, std::string(name)) {

}

InstructionPacketFIFO::~InstructionPacketFIFO() {

}
