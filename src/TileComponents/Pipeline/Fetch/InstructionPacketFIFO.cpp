/*
 * InstructionPacketFIFO.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketFIFO.h"
#include "../../../Datatype/Instruction.h"

const Instruction InstructionPacketFIFO::read() {
  Instruction inst = fifo.read();
  sendCredit();
  return inst;
}

void InstructionPacketFIFO::write(const Instruction inst) {
  fifo.write(inst);
  if(DEBUG) cout << this->name() << " received Instruction:  " << inst << endl;
}

void InstructionPacketFIFO::sendCredit() {
  flowControl.write(1);
}

bool InstructionPacketFIFO::isEmpty() const {
  return fifo.empty();
}

InstructionPacketFIFO::InstructionPacketFIFO(sc_module_name name) :
    Component(name),
    fifo(IPK_FIFO_SIZE, std::string(name)) {

}

InstructionPacketFIFO::~InstructionPacketFIFO() {

}
