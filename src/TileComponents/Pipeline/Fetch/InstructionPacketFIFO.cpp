/*
 * InstructionPacketFIFO.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketFIFO.h"

void InstructionPacketFIFO::insert() {
  Instruction i = in.read();
  fifo.write(i);
  wroteSig.write(!wroteSig.read());
}

void InstructionPacketFIFO::remove() {
  if(!fifo.isEmpty()) {
    out.write(fifo.read());
    readSig.write(!readSig.read());
  }
}

void InstructionPacketFIFO::updateEmptySig() {
  empty.write(isEmpty());
}

void InstructionPacketFIFO::newCycle() {
  flowControl.write(!fifo.isFull());
}

bool InstructionPacketFIFO::isEmpty() {
  return fifo.isEmpty();
}

InstructionPacketFIFO::InstructionPacketFIFO(sc_module_name name) :
    Component(name),
    fifo(IPK_FIFO_SIZE) {

// Register methods
  SC_METHOD(insert);
  sensitive << in;
  dont_initialize();

  SC_METHOD(remove);
  sensitive << readInstruction;
  dont_initialize();

  SC_METHOD(updateEmptySig);
  sensitive << readSig << wroteSig;
  // do initialise

  SC_METHOD(newCycle);
  sensitive << clock.pos();
  // do initialise

}

InstructionPacketFIFO::~InstructionPacketFIFO() {

}
