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

class InstructionPacketFIFO : public Component {

//==============================//
// Ports
//==============================//

public:

  // Signal telling the flow control unit how much space is left in the FIFO.
  sc_out<int>         flowControl;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(InstructionPacketFIFO);
  InstructionPacketFIFO(sc_core::sc_module_name name);
  virtual ~InstructionPacketFIFO();

//==============================//
// Methods
//==============================//

public:

  Instruction read();
  void write(Instruction inst);
  bool isEmpty();

private:

  void updateFlowControl();

//==============================//
// Local state
//==============================//

private:

  Buffer<Instruction> fifo;

};

#endif /* INSTRUCTIONPACKETFIFO_H_ */
