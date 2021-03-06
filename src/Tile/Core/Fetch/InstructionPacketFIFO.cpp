/*
 * InstructionPacketFIFO.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketFIFO.h"

#include "../../../Datatype/Instruction.h"
#include "../../../Utility/Instrumentation/Latency.h"
#include "FetchStage.h"

const Instruction InstructionPacketFIFO::read() {
  Instruction inst = fifo.read().payload();
  fifoRead.notify(sc_core::SC_ZERO_TIME);

  CacheIndex readPointer = fifo.getReadPointer();
  lastReadAddr = addresses[readPointer];

  return inst;
}

void InstructionPacketFIFO::write(const Instruction inst) {
  // TODO: use a BandwidthMonitor for data written to the FIFO (like in
  // NetworkFIFO). I haven't done this for now because all possible sources of
  // data are currently NetworkFIFOs with their own BandwidthMonitors, and there
  // can be at most one writer at a time.

  LOKI_LOG(3) << this->name() << " received Instruction:  " << inst << endl;
  parent().fifoInstructionArrived(inst);

  // If this is a "next instruction packet" command, don't write it to the FIFO,
  // but instead immediately move to the next packet, if there is one.
  if (inst.opcode() == ISA::OP_NXIPK) {
    parent().nextIPK();
    return;
  }

  CacheIndex writePos = fifo.getWritePointer();

  // A NetworkFIFO needs flits, not raw instructions. Add a dummy address.
  Flit<Word> flit(inst, ChannelID());

  fifo.write(flit);
  fifoWrite.notify(sc_core::SC_ZERO_TIME);

  if (finishedPacketWrite) {  // Start of new packet
    InstLocation location;
    location.component = IPKFIFO;
    location.index = writePos;
    tag = parent().newPacketArriving(location);
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
    parent().packetFinishedArriving(IPKFIFO);
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
  return !fifo.canRead() && !tagMatched;
}

bool InstructionPacketFIFO::isFull() const{
  return !fifo.canWrite();
}

const sc_event& InstructionPacketFIFO::readEvent() const {
  return fifoRead;
}

const sc_event& InstructionPacketFIFO::writeEvent() const {
  return fifoWrite;
}

FetchStage& InstructionPacketFIFO::parent() const {
  return static_cast<FetchStage&>(*(this->get_parent_object()));
}

void InstructionPacketFIFO::write(const Flit<Word>& data) {
  Instrumentation::Latency::coreReceivedResult(parent().id(), data);

  // Strip off the network address and store the instruction.
  Instruction inst = static_cast<Instruction>(data.payload());
  write(inst);
}

bool InstructionPacketFIFO::canWrite() const {
  return !isFull();
}

const sc_event& InstructionPacketFIFO::canWriteEvent() const {
  return fifo.canWriteEvent();
}

const sc_event& InstructionPacketFIFO::dataConsumedEvent() const {
  return fifo.dataConsumedEvent();
}

const Flit<Word> InstructionPacketFIFO::lastDataWritten() const {
  return fifo.lastDataWritten();
}

InstructionPacketFIFO::InstructionPacketFIFO(sc_module_name name,
                                             const fifo_parameters_t& params) :
    LokiComponent(name),
    clock("clock"),
    fifo("fifo", params),
    addresses(params.size, DEFAULT_TAG) {

  fifo.clock(clock);

  tag = DEFAULT_TAG;

  lastReadAddr = 0;
  lastWriteAddr = 0;

  // Ensure that the first instruction to arrive gets queued up to execute.
  finishedPacketWrite = true;

  tagMatched = false;
  startOfPacket = NOT_IN_CACHE;

}
