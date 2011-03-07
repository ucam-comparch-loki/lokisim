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
#include "../../../Memory/BufferStorage.h"

class Instruction;

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

  InstructionPacketFIFO(sc_module_name name);
  virtual ~InstructionPacketFIFO();

//==============================//
// Methods
//==============================//

public:

  // Read a single instruction from the end of the FIFO.
  const Instruction read();

  // Write an instruction to the end of the FIFO.
  void write(const Instruction inst);

  // Returns whether the FIFO is empty.
  bool isEmpty() const;

private:

  void sendCredit();

//==============================//
// Local state
//==============================//

private:

  BufferStorage<Instruction> fifo;

};

#endif /* INSTRUCTIONPACKETFIFO_H_ */
