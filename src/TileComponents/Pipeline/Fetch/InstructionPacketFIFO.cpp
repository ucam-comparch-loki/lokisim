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
  fifoRead.notify(sc_core::SC_ZERO_TIME);

  CacheIndex readPointer = fifo.getReadPointer();
  lastReadAddr = addresses[readPointer];

  return inst;
}

void InstructionPacketFIFO::write(const Instruction inst) {
  LOKI_LOG << this->name() << " received Instruction:  " << inst << endl;

  // If this is a "next instruction packet" command, don't write it to the FIFO,
  // but instead immediately move to the next packet, if there is one.
  if (inst.opcode() == InstructionMap::OP_NXIPK) {
    parent()->nextIPK();
    return;
  }

  CacheIndex writePos = fifo.getWritePointer();

  fifo.write(inst);
  fifoWrite.notify(sc_core::SC_ZERO_TIME);

  if (finishedPacketWrite) {  // Start of new packet
    InstLocation location;
    location.component = IPKFIFO;
    location.index = writePos;
    tag = parent()->newPacketArriving(location);
    startOfPacket = writePos;
    lastWriteAddr = tag;
  }
  else {
    lastWriteAddr += BYTES_PER_WORD;

    if (writePos == startOfPacket) { // Overwriting previous start of packet
      startOfPacket = NOT_IN_CACHE;
      tag = DEFAULT_TAG;
    }
  }

  addresses[writePos] = lastWriteAddr;

  finishedPacketWrite = inst.endOfPacket();
  if (finishedPacketWrite)
    parent()->packetFinishedArriving(IPKFIFO);
}

CacheIndex InstructionPacketFIFO::lookup(MemoryAddr tag) {
  // Temporarily commented out because the FIFO is much bigger than it should
  // be, and would skew the results if it worked as a cache.
  if (this->tag == tag) {
    fifoWrite.notify(sc_core::SC_ZERO_TIME); // A successful lookup means the FIFO isn't empty
    tagMatched = true;
    return startOfPacket;
  }
  else
    return NOT_IN_CACHE;
}

MemoryAddr InstructionPacketFIFO::memoryAddress() const {
  return lastReadAddr;
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
  fifo.setReadPointer(fifo.getReadPointer() + amount - 1);
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

const sc_event& InstructionPacketFIFO::readEvent() const {
  return fifoRead;
}

const sc_event& InstructionPacketFIFO::writeEvent() const {
  return fifoWrite;
}

void InstructionPacketFIFO::updateReady() {
  oFlowControl.write(!fifo.full());
}

void InstructionPacketFIFO::dataConsumedAction() {
  oDataConsumed.write(!oDataConsumed.read());
}

FetchStage* InstructionPacketFIFO::parent() const {
  return static_cast<FetchStage*>(this->get_parent_object());
}

InstructionPacketFIFO::InstructionPacketFIFO(sc_module_name name) :
    Component(name),
    fifo(std::string(name), IPK_FIFO_SIZE),
    addresses(IPK_FIFO_SIZE, DEFAULT_TAG) {

  tag = DEFAULT_TAG;

  lastReadAddr = 0;
  lastWriteAddr = 0;

  // Ensure that the first instruction to arrive gets queued up to execute.
  finishedPacketWrite = true;

  tagMatched = false;
  startOfPacket = NOT_IN_CACHE;

  SC_METHOD(updateReady);
  sensitive << fifoRead << fifoWrite;
  // do initialise

  SC_METHOD(dataConsumedAction);
  sensitive << fifo.dataConsumedEvent();
  dont_initialize();

}
