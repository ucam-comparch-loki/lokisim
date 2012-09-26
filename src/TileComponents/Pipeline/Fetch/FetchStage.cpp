/*
 * FetchStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "FetchStage.h"
#include "../../Cluster.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Utility/Instrumentation/IPKCache.h"

void FetchStage::execute() {
  bool isIdle = false;
  bool waiting = waitingForInstructions();

  // Instrumentation stuff
  // Idle = have no work to do; stalled = waiting for work to arrive
  if (currentPacket.active() && !finishedPacketRead)
    if (waiting)
      Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_INSTRUCTIONS);
    else
      Instrumentation::Stalls::unstall(id);
  else
    isIdle = true;

  // Update the idle signal if there was a change.
  if (idle.read() != isIdle)
    idle.write(isIdle);

  // Find an instruction to pass to the pipeline.
  if (waitingForInstructions()) {          // Wait for an instruction
    next_trigger(fifo.fillChangedEvent() | cache.fillChangedEvent());
  }
  else if (!canSendInstruction()) {        // Wait until decoder is ready
    next_trigger(canSendEvent());
  }
  else if (!clock.negedge()) {             // Do fetch at negedge (why?)
//  else if (clock.read()) {               // TODO: this is better - find out why
    next_trigger(clock.negedge_event());
  }
  else {                                   // Pass an instruction to pipeline
    getInstruction();
    next_trigger(clock.negedge_event());
  }
}

bool FetchStage::waitingForInstructions() {
  // An empty component can appear non-empty if it has jumped back to a previous
  // packet - only want to rely on isEmpty if in the middle of a packet.

  if (jumpedThisCycle)
    return false;

  // Hack to tell if we're executing instructions from another core - they all
  // appear as separate packets, so we need to check the FIFO instead.
  if (finishedPacketRead && currentPacket.location.component == IPKFIFO &&
      currentPacket.memAddr == 0 && !fifo.isEmpty()) {
    finishedPacketRead = false;
    return false;
  }

  if (finishedPacketRead && currentPacket.persistent)
    return false;

  if (finishedPacketRead)
    return pendingPacket.location.index == NOT_IN_CACHE;

  if (currentPacket.location.component == UNKNOWN)
    return true;
  else
    return currentInstructionSource().isEmpty();
}

void FetchStage::getInstruction() {
  // Select a new instruction if the decode stage is ready for one, unless
  // the FIFO and cache are both empty.

  assert(canSendInstruction());

  if (finishedPacketRead) {
    switchToPendingPacket();
    currentAddr = currentPacket.memAddr;
  }

  MemoryAddr instAddr = currentAddr;
  currentAddr += BYTES_PER_WORD;
  Instruction instruction;

  InstructionStore& source = currentInstructionSource();
  if (source.isEmpty()) {
    next_trigger(source.fillChangedEvent());
    return;
  }
  else
    instruction = source.read();

  // The Instruction becomes a DecodedInst here to simplify various interfaces
  // throughout the pipeline. The decoding actually happens in the decode stage.
  DecodedInst decoded(instruction);
  decoded.location(instAddr);

  if (DEBUG) {
    static const string names[] = {"FIFO", "cache", "unknown"};
    cout << this->name() << " selected instruction from "
         << names[currentPacket.location.component] << ": " << decoded << endl;
  }

  // Make sure we didn't read a junk instruction. "nor r0, r0, r0 -> 0" seems
  // pretty unlikely to ever come up in a real program.
  assert(instruction.toInt() != 0);

  if (decoded.endOfIPK())
    reachedEndOfPacket();

  jumpedThisCycle = false;

  outputInstruction(decoded);
  instructionCompleted();
}

void FetchStage::updateReady() {
  // Consider the pipeline to be stalled if the first pipeline stage is not
  // allowed to do any work. Only report the stall status when it changes.

  if (canSendInstruction() == stalled) {
    stalled = !canSendInstruction();
    parent()->pipelineStalled(stalled);
  }
}

void FetchStage::storeCode(const std::vector<Instruction>& instructions) {
  // Find the first packet which hasn't finished arriving, so we can add to it.
  PacketInfo* packet;
  if (!currentPacket.inCache)
    packet = &currentPacket;
  else {
    assert(!pendingPacket.inCache);
    packet = &pendingPacket;
  }

  if (packet->memAddr == DEFAULT_TAG)
    packet->memAddr = 0;

  cache.storeCode(instructions);

  idle.write(false);
}

MemoryAddr FetchStage::getInstIndex() const {
  return currentAddr;
}

bool FetchStage::canCheckTags() const {
  // Can only prepare for another packet if there is a space to store its
  // information.
  return !pendingPacket.active();
}

bool FetchStage::inCache(const MemoryAddr addr, opcode_t operation) {
  if (DEBUG)
    cout << this->name() << " looking up tag " << addr << ": ";

  // If we are looking for a packet, we are going to want to execute it at some
  // point. Find out where in the execution queue the packet should go.
  PacketInfo& packet = nextAvailablePacket();

  // Find out where, if anywhere, the packet is located.
  packet.location   = lookup(addr);

  packet.memAddr    = addr;
  packet.inCache    = packet.location.index != NOT_IN_CACHE;
  packet.execute    = operation != InstructionMap::OP_FILL &&
                      operation != InstructionMap::OP_FILLR;
  packet.persistent = operation == InstructionMap::OP_FETCHPST ||
                      operation == InstructionMap::OP_FETCHPSTR;

  bool found = packet.inCache;

  if (DEBUG)
    cout << (found ? "" : "not ") << "cached" << endl;

  // Clear the packet information if we aren't going to do anything with it.
  // This may be the case if we are prefilling the cache with instructions.
  if (packet.inCache && !packet.execute)
    packet.reset();

  if (ENERGY_TRACE)
    Instrumentation::IPKCache::tagCheck(id, packet.inCache);

  return found;
}

const InstLocation FetchStage::lookup(const MemoryAddr addr) {
  InstLocation location;

  if ((location.index = fifo.lookup(addr)) != NOT_IN_CACHE)
    location.component = IPKFIFO;
  else if ((location.index = cache.lookup(addr)) != NOT_IN_CACHE)
    location.component = IPKCACHE;
  else
    location.component = UNKNOWN; // Wait for packet to start arriving before we know

  return location;
}

bool FetchStage::roomToFetch() const {
  // We only allow a new fetch request to be sent when there is room in the
  // cache, and there is no packet currently in transfer (we don't want to
  // interleave the instructions).
  return cache.roomToFetch() && (!currentPacket.active() || currentPacket.inCache);
}

/* Perform any status updates required when we receive a position to jump to. */
void FetchStage::jump(const JumpOffset offset) {
  if (DEBUG)
    cout << this->name() << " jumped by " << offset << endl;

  finishedPacketRead = false;
  jumpedThisCycle = true;
  currentAddr += offset;// * BYTES_PER_WORD;
  currentInstructionSource().jump(offset);
}

void FetchStage::nextIPK() {
  currentPacket.persistent = false;
  finishedPacketRead = true;

  // Stop the current packet executing if it hasn't finished arriving yet.
  if (currentPacket.arriving())
    currentPacket.execute = false;

  if (currentPacket.location.component != UNKNOWN)
    currentInstructionSource().cancelPacket();

  parent()->nextIPK();
}

MemoryAddr FetchStage::newPacketArriving(const InstLocation& location) {
  // Check for special case of one core sending instructions to another. All
  // instructions will look like separate packets, so we want to merge them.
  if (location.component == IPKFIFO) {
    if (pendingPacket.location.component == IPKFIFO && pendingPacket.memAddr == 0)
      return 0;
    else if (!pendingPacket.active() &&
             currentPacket.location.component == IPKFIFO && currentPacket.memAddr == 0)
      return 0;
  }

  // Look for the first packet in the queue which we don't know the location of.
  PacketInfo* packet;
  if (currentPacket.location.index == NOT_IN_CACHE)
    packet = &currentPacket;
  else {
    assert(pendingPacket.location.index == NOT_IN_CACHE);
    packet = &pendingPacket;
  }

  // Associate that packet with the location we just received.
  packet->location = location;

  // If we weren't expecting this packet (it was requested by another core,
  // or by the debugger), we won't have a tag, so provide one.
  if (packet->memAddr == DEFAULT_TAG)
    packet->memAddr = 0;

  // Return the memory address, so the instruction store can give the packet a
  // tag.
  return packet->memAddr;
}

void FetchStage::packetFinishedArriving() {
  // Look for the first packet in the queue which we don't have.
  PacketInfo* packet;
  if (currentPacket.inCache)
    packet = &pendingPacket;
  else
    packet = &currentPacket;

  packet->inCache = true;

  // If we are not due to execute this packet, clear its entry in the queue so
  // we can fetch another.
  if (!packet->execute)
    packet->reset();
}

void FetchStage::reachedEndOfPacket() {
  // Note: we can't actually switch to the next packet yet in case there is a
  // jump in this packet which we haven't executed yet. Instead, set a flag so
  // we can switch when we are sure that there are no jumps.
  if (!jumpedThisCycle)
    finishedPacketRead = true;
}

void FetchStage::switchToPendingPacket() {
  if (!currentPacket.persistent) {
    currentPacket = pendingPacket;
    pendingPacket.reset();
  }

  if (currentPacket.location.component != UNKNOWN)
    currentInstructionSource().startNewPacket(currentPacket.location.index);

  finishedPacketRead = false;

  if (DEBUG) {
    static const string names[] = {"FIFO", "cache", "unknown"};
    cout << this->name() << " switched to pending packet: " << names[currentPacket.location.component] <<
      ", position " << currentPacket.location.index << " (" << currentPacket.memAddr << ")" << endl;
  }
}

FetchStage::PacketInfo& FetchStage::nextAvailablePacket() {
  if (currentPacket.active()) {
    assert(!pendingPacket.active());
    return pendingPacket;
  }
  else
    return currentPacket;
}

InstructionStore& FetchStage::currentInstructionSource() {
  assert(currentPacket.location.component != UNKNOWN);

  if (currentPacket.location.component == IPKFIFO)
    return fifo;
  else if (currentPacket.location.component == IPKCACHE)
    return cache;
  else
    assert(false && "Invalid instruction source.");
}

void FetchStage::getNextInstruction() {
  // This pipeline stage is dedicated to getting instructions, so call the main
  // method from here.
  execute();
}

FetchStage::FetchStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    cache("IPKcache", ID),
    fifo("IPKfifo") {

  stalled     = false;  // Start off idle, but not stalled.
  flowControl.init(2);

  currentPacket.reset(); pendingPacket.reset();

  // Connect FIFO and cache to network
  fifo.clock(clock);
  fifo.instructionIn(toIPKFIFO);
  fifo.flowControl(flowControl[0]);
  cache.clock(clock);
  cache.instructionIn(toIPKCache);
  cache.flowControl(flowControl[1]);

  SC_METHOD(updateReady);
  sensitive << /*canSendEvent() << FIXME*/ instructionCompletedEvent;
  // do initialise

}
