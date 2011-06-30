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
  fifoFillChanged.notify();
  return inst;
}

void InstructionPacketFIFO::write(const Instruction inst) {
  fifo.write(inst);
  fifoFillChanged.notify();
  if(DEBUG) cout << this->name() << " received Instruction:  " << inst << endl;
}

bool InstructionPacketFIFO::isEmpty() const {
  return fifo.empty();
}

const sc_core::sc_event& InstructionPacketFIFO::fillChangedEvent() const {
  return fifoFillChanged;
}

void InstructionPacketFIFO::receivedInst() {
  // Need to cast input Word to Instruction.
  Instruction inst = static_cast<Instruction>(instructionIn.read());
  write(inst);
}

void InstructionPacketFIFO::updateReady() {
  flowControl.write(!fifo.full());
}

InstructionPacketFIFO::InstructionPacketFIFO(sc_module_name name) :
    Component(name),
    fifo(IPK_FIFO_SIZE, std::string(name)) {

  SC_METHOD(receivedInst);
  sensitive << instructionIn;
  dont_initialize();

  SC_METHOD(updateReady);
  sensitive << fifoFillChanged;
  // do initialise

}
