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

class FetchStage;
class Instruction;
class Word;

class InstructionPacketFIFO : public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool> clock;

  sc_in<Word> instructionIn;

  // Signal telling the flow control unit whether there is space left in the FIFO.
  sc_out<bool> flowControl;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(InstructionPacketFIFO);
  InstructionPacketFIFO(sc_module_name name);

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

  // A handle to an event which is triggered whenever an instruction is added
  // to or removed from the FIFO.
  const sc_core::sc_event& fillChangedEvent() const;

private:

  void receivedInst();
  void updateReady();

  FetchStage* parent() const;

//==============================//
// Local state
//==============================//

private:

  BufferStorage<Instruction> fifo;

  // An event which is triggered whenever an instruction is read from or
  // written to the FIFO.
  sc_core::sc_event fifoFillChanged;

};

#endif /* INSTRUCTIONPACKETFIFO_H_ */
