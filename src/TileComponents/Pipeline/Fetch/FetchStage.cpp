/*
 * FetchStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "FetchStage.h"
#include "../../Core.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Utility/Instrumentation/IPKCache.h"

void FetchStage::execute() {
  bool waiting = waitingForInstructions();

  // Instrumentation stuff
  // Idle = have no work to do; stalled = waiting for work to arrive
  if (currentPacket.active() && !finishedPacketRead) {
    if (waiting)
      Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_INSTRUCTIONS);
    else
      Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_INSTRUCTIONS);
  }

  // Find an instruction to pass to the pipeline.
  if (waiting) {                           // Wait for an instruction
    next_trigger(fifo.fillChangedEvent() | cache.fillChangedEvent());
  }
  else if (!canSendInstruction()) {        // Wait until decoder is ready
    next_trigger(canSendEvent());
  }
  else if (!clock.negedge()) {             // Do fetch at negedge (why?)
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

  // TODO: if the current packet isn't going to be executed, wait until it
  // finishes arriving or switch to the pending packet.

  if (finishedPacketRead) {
    // See if we're due to execute the same packet again.
    if (currentPacket.persistent)
      return false;

    // See if the next packet has already arrived.
    if (pendingPacket.location.index != NOT_IN_CACHE)
      return false;

    // Hack: special case for receiving instructions from another core - they
    // all appear as separate packets, so we haven't really finishedPacketRead.
    if (currentPacket.location.component == IPKFIFO &&
        currentPacket.memAddr == 0 && !fifo.isEmpty()) {
      finishedPacketRead = false;
      return false;
    }

    return true;
  }

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
  Instruction instruction;

  InstructionStore& source = currentInstructionSource();
  if (needRefetch) {
    instruction = Instruction("fetchr.eop 0");
    needRefetch = false;

    if (DEBUG)
      cout << this->name() << ": packet overwritten; need to refetch" << endl;
  }
  else if (source.isEmpty()) {
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

  //fprintf(stderr, "0x%08x\n", instAddr);

  // Make sure we didn't read a junk instruction. "nor r0, r0, r0 -> 0" seems
  // pretty unlikely to ever come up in a real program.
  assert(instruction.toInt() != 0);

  if (decoded.endOfIPK()) {
    reachedEndOfPacket();

    // Check for the special case of single-instruction persistent packets.
    // These do not need to be read repeatedly, so remove the persistent flag.
    if (currentPacket.persistent && currentPacket.memAddr == currentAddr) {
      decoded.persistent(true);
      currentPacket.persistent = false;
    }
  }

  jumpedThisCycle = false;
  currentAddr += BYTES_PER_WORD;

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
    cout << this->name() << " looking up tag 0x" << std::hex << addr << std::dec << ": ";

  // I suppose the address could technically be 0, but this always indicates an
  // error with the current arrangement.
  assert(addr > 0);

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

  // If we're in persistent execution mode, but just fetched a new packet, stop
  // executing this packet.
  // Note that the fetch must currently be the last instruction of the packet.
  if (packet.execute && currentPacket.persistent)
    nextIPK();

  if (ENERGY_TRACE)
    Instrumentation::IPKCache::tagCheck(id, packet.inCache, addr, previousFetch);

  previousFetch = addr;

  if (!packet.inCache) {
    Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_INSTRUCTIONS);
  }

  return found;
}

const InstLocation FetchStage::lookup(const MemoryAddr addr) {
  InstLocation location;

  // Make sure we do the look-up in both locations, so we record the energy
  // consumption of both, rather than just the first component to find the tag.
  CacheIndex fifoPos = fifo.lookup(addr);
  CacheIndex cachePos = cache.lookup(addr);

  if ((location.index = fifoPos) != NOT_IN_CACHE)
    location.component = IPKFIFO;
  else if ((location.index = cachePos) != NOT_IN_CACHE)
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

  Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_INSTRUCTIONS);

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
  if (!packet->execute) {
    packet->reset();

    // FIXME: not waiting for fill to complete before starting to execute
//    if (packet == &currentPacket) {
//      currentPacket = pendingPacket;
//      pendingPacket.reset();
//    }
  }
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

  // Check to make sure the packet is still there - between the tag check and
  // now, more instructions may have arrived and overwritten the packet.
  if (!currentInstructionSource().packetExists(currentPacket.location.index))
    needRefetch = true;

  finishedPacketRead = false;

  if (DEBUG) {
    static const string names[] = {"FIFO", "cache", "unknown"};
    cout << this->name() << " switched to pending packet: " << names[currentPacket.location.component] <<
      ", position " << currentPacket.location.index << " (0x" << std::hex << currentPacket.memAddr << std::dec << ")" << endl;
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
  else {
    assert(false && "Invalid instruction source.");
    throw -1;
  }
}

void FetchStage::getNextInstruction() {
  // This pipeline stage is dedicated to getting instructions, so call the main
  // method from here.
  execute();
}

void FetchStage::reportStalls(ostream& os) {
  if (currentPacket.active() && !currentPacket.inCache) {
    os << this->name() << " waiting for instructions from 0x" << std::hex << currentPacket.memAddr << std::dec << endl;
    InstructionStore& source = currentInstructionSource();
    os << "  Storing instructions in " << ((currentPacket.location.component == IPKFIFO) ? "FIFO" : "IPK cache") << endl;
    if (source.isFull())
      os << "  No space remaining." << endl;
  }
}

FetchStage::FetchStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    cache("IPKcache", ID),
    fifo("IPKfifo") {

  stalled     = false;  // Start off idle, but not stalled.
  jumpedThisCycle = false;
  finishedPacketRead = false;
  needRefetch = false;

  oFlowControl.init(2);
  oDataConsumed.init(2);

  currentPacket.reset(); pendingPacket.reset();

  // Connect FIFO and cache to network
  fifo.clock(clock);
  fifo.iInstruction(iToFIFO);
  fifo.oFlowControl(oFlowControl[0]);
  fifo.oDataConsumed(oDataConsumed[0]);
  cache.clock(clock);
  cache.iInstruction(iToCache);
  cache.oFlowControl(oFlowControl[1]);
  cache.oDataConsumed(oDataConsumed[1]);

  SC_METHOD(updateReady);
  sensitive << /*canSendEvent() << FIXME*/ instructionCompletedEvent;
  // do initialise

}
