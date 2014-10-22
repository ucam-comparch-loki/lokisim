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
#include "InstructionStore.h"
#include "../../../Network/NetworkBuffer.h"
#include "../../../Network/NetworkTypedefs.h"

class FetchStage;
class Instruction;
class Word;

class InstructionPacketFIFO : public Component, public InstructionStore {

//==============================//
// Ports
//==============================//

public:

  ClockInput  clock;

  sc_in<Word> iInstruction;

  // Signal telling the flow control unit whether there is space left in the FIFO.
  ReadyOutput oFlowControl;

  // Signal which toggles whenever data has been consumed.
  ReadyOutput oDataConsumed;

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
  virtual const Instruction read();

  // Write an instruction to the end of the FIFO.
  virtual void write(const Instruction inst);

  // Return the position within the FIFO that the instruction with the given
  // tag is, or NOT_IN_CACHE if it is not there.
  virtual CacheIndex lookup(MemoryAddr tag);

  // Return the memory address of the most recently read instruction. Used for
  // debug purposes only.
  virtual MemoryAddr memoryAddress() const;

  // Returns whether there is still an instruction packet at the given position.
  // In rare circumstances, the packet may be overwritten between first checking
  // for it and beginning to execute it.
  virtual bool packetExists(CacheIndex position) const;

  // Prepare to read a packet with the first instruction at the given location
  // within the FIFO.
  virtual void startNewPacket(CacheIndex position);

  // Perform any necessary tidying when an instruction packet is aborted.
  virtual void cancelPacket();

  // Immediately jump to a relative position in the FIFO.
  virtual void jump(JumpOffset amount);

  // Return whether the FIFO is empty.
  virtual bool isEmpty() const;
  virtual bool isFull() const;

  // A handle to an event which is triggered whenever an instruction is added
  // to or removed from the FIFO.
  virtual const sc_event& fillChangedEvent() const;

private:

  void receivedInst();
  void updateReady();

  void dataConsumedAction();

  FetchStage* parent() const;

//==============================//
// Local state
//==============================//

private:

  NetworkBuffer<Instruction> fifo;

  // The FIFO holds a single tag, so it is able to efficiently hold very
  // tight loops consisting of a single packet.
  MemoryAddr tag;

  // Memory addresses of each instruction in the cache. For debug purposes only.
  vector<MemoryAddr> addresses;

  // Information used to keep track of the memory addresses of instructions.
  MemoryAddr lastReadAddr, lastWriteAddr;

  // Record whether there has been a tag check which matched our tag - this
  // can affect wither the FIFO appears empty or not.
  bool tagMatched;

  // The position in the FIFO of the packet associated with the single tag.
  CacheIndex startOfPacket;

  // An event which is triggered whenever an instruction is read from or
  // written to the FIFO.
  sc_event fifoFillChanged;

  // Flag used to determine when a new instruction packet starts arriving.
  bool finishedPacketWrite;

};

#endif /* INSTRUCTIONPACKETFIFO_H_ */
