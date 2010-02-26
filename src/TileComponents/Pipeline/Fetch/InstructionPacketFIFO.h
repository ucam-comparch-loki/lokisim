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
  sc_in<bool>         clock, readInstruction;
  sc_in<Instruction>  in;
  sc_out<bool>        empty, flowControl;
  sc_out<Instruction> out;

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
