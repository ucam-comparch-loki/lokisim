/*
 * InstructionPacketFIFO.h
 *
 * Buffer of instructions received from the network.
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#ifndef INSTRUCTIONPACKETFIFO_H_
#define INSTRUCTIONPACKETFIFO_H_

#include "../../../Component.h"
#include "../../../Memory/Buffer.h"
#include "../../../Datatype/Instruction.h"

class InstructionPacketFIFO : public Component {

public:

/* Ports */

  // Clock.
  sc_in<bool>         clock;

  // The instruction received over the network.
  sc_in<Instruction>  in;

  // Signal which changes to tell the FIFO to emit the next instruction.
  sc_in<bool>         readInstruction;

  // The instruction read from the FIFO.
  sc_out<Instruction> out;

  // Signals whether or not the FIFO is empty.
  sc_out<bool>        empty;

  // Signals to the network whether there is room to put the next instruction
  // in the FIFO.
  sc_out<bool>        flowControl;

/* Constructors and destructors */
  SC_HAS_PROCESS(InstructionPacketFIFO);
  InstructionPacketFIFO(sc_core::sc_module_name name);
  virtual ~InstructionPacketFIFO();

private:
/* Methods */
  void insert();
  void remove();
  void updateEmptySig();
  void newCycle();
  bool isEmpty();

/* Local state */
  Buffer<Instruction> fifo;

/* Signals (wires) */
  sc_signal<bool>     readSig, wroteSig;

};

#endif /* INSTRUCTIONPACKETFIFO_H_ */
