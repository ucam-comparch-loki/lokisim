/*
 * InstructionPacketFIFO.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketFIFO.h"
#include "FetchStage.h"
#include "../../../Datatype/Instruction.h"

const Instruction InstructionPacketFIFO::read() {
  Instruction inst = fifo.read();
  fifoFillChanged.notify();
  return inst;
}

void InstructionPacketFIFO::write(const Instruction inst) {
  CacheIndex writePos = fifo.getWritePointer();

  fifo.write(inst);
  fifoFillChanged.notify();

  if (finishedPacketWrite) {  // Start of new packet
    InstLocation location;
    location.component = IPKFIFO;
    location.index = writePos;
    tag = parent()->newPacketArriving(location);
    startOfPacket = writePos;
  }
  else if (writePos == startOfPacket) { // Overwriting previous start of packet
    startOfPacket = NOT_IN_CACHE;
    tag = DEFAULT_TAG;
  }

  finishedPacketWrite = inst.endOfPacket();

  if (finishedPacketWrite)
    parent()->packetFinishedArriving();
}

CacheIndex InstructionPacketFIFO::lookup(MemoryAddr tag) {
  // Temporarily commented out because the FIFO is much bigger than it should
  // be, and would skew the results if it worked as a cache.
  if (this->tag == tag) {
    fifoFillChanged.notify();
    tagMatched = true;
    return startOfPacket;
  }
  else
    return NOT_IN_CACHE;
}

bool InstructionPacketFIFO::packetExists(CacheIndex position) const {
  return position == startOfPacket;
}

void InstructionPacketFIFO::startNewPacket(CacheIndex position) {
  fifo.setReadPointer(position);
  tagMatched = false;
}

void InstructionPacketFIFO::cancelPacket() {
  // TODO
  fifo.setReadPointer(fifo.getWritePointer());
}

void InstructionPacketFIFO::jump(JumpOffset amount) {
  // Do we need to do something with tagMatched here too?
  fifo.setReadPointer(fifo.getReadPointer() + amount/BYTES_PER_WORD - 1);
  // Is the -1 a hack?
}

bool InstructionPacketFIFO::isEmpty() const {
  // If we have a packet which is due to be executed soon, the FIFO is not
  // empty.
  return fifo.empty() && !tagMatched;
}

bool InstructionPacketFIFO::isFull() const{
  return fifo.full();
}

const sc_event& InstructionPacketFIFO::fillChangedEvent() const {
  return fifoFillChanged;
}

void InstructionPacketFIFO::receivedInst() {
  // Need to cast input Word to Instruction.
  Instruction inst = static_cast<Instruction>(iInstruction.read());
  if(DEBUG) cout << this->name() << " received Instruction:  " << inst << endl;

  // If this is a "next instruction packet" command, don't write it to the FIFO,
  // but instead immediately move to the next packet, if there is one.
  if(inst.opcode() == InstructionMap::OP_NXIPK)
    parent()->nextIPK();
  else
    write(inst);
}

void InstructionPacketFIFO::updateReady() {
  oFlowControl.write(!fifo.full());
}

void InstructionPacketFIFO::dataConsumedAction() {
  oDataConsumed.write(!oDataConsumed.read());
}

FetchStage* InstructionPacketFIFO::parent() const {
  return static_cast<FetchStage*>(this->get_parent());
}

InstructionPacketFIFO::InstructionPacketFIFO(sc_module_name name) :
    Component(name),
    fifo(IPK_FIFO_SIZE, std::string(name)) {

  // Ensure that the first instruction to arrive gets queued up to execute.
  finishedPacketWrite = true;

  tagMatched = false;

  SC_METHOD(receivedInst);
  sensitive << iInstruction;
  dont_initialize();

  SC_METHOD(updateReady);
  sensitive << fifoFillChanged;
  // do initialise

  SC_METHOD(dataConsumedAction);
  sensitive << fifo.dataConsumedEvent();
  dont_initialize();

}
