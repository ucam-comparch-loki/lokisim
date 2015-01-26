/*
 * FetchStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "FetchStage.h"
#include "../../Core.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/Instrumentation/IPKCache.h"
#include "../../../Exceptions/InvalidOptionException.h"

void FetchStage::execute() {
  // Do nothing - everything is handled in readLoop and writeLoop.
}

// Continually read instructions from the current instruction packet and pass
// them to the rest of the pipeline. When the packet completes, switch to the
// pending packet, when available.
void FetchStage::readLoop() {

  switch (readState) {
    case RS_READY: {
      // Have a packet fetched and ready to go.
      if (pendingPacket.active() && pendingPacket.execute) {
        currentPacket = pendingPacket;

        if (currentPacket.location.index == NOT_IN_CACHE) {
          Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_INSTRUCTIONS, DecodedInst());
          next_trigger(currentInstructionSource().fillChangedEvent());
          break;
        }
        else {
          currentInstructionSource().startNewPacket(currentPacket.location.index);

          if (DEBUG) {
            static const string names[] = {"FIFO", "cache", "unknown"};
            cout << this->name() << " switched to pending packet: " << names[currentPacket.location.component] <<
              ", position " << currentPacket.location.index << " (0x" << std::hex << currentPacket.memAddr << std::dec << ")" << endl;
          }

          pendingPacket.reset();
          readState = RS_READ;
          // Fall through to next state.
        }

      }
      // There are unexecuted instructions in the FIFO.
      else if (!fifo.isEmpty())
        break;
      // Nothing to do - wait until a new instruction packet arrives.
      else {
        // Don't stall just because we don't have a packet to execute. Only
        // stall once we've executed a fetch and are expecting more instructions.
        next_trigger(newPacketAvailable);
        break;
      }
    }
    // no break

    // TODO: nextIPK needs to break us out of this state
    case RS_READ: {
      if (currentInstructionSource().isEmpty()) {
        Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_INSTRUCTIONS, DecodedInst());
        next_trigger(currentInstructionSource().fillChangedEvent());
      }
      else if (!canSendInstruction())
        next_trigger(canSendEvent());
      else if (!clock.negedge())
        next_trigger(clock.negedge_event());
      else {
        // The Instruction becomes a DecodedInst here to simplify various interfaces
        // throughout the pipeline. The decoding actually happens in the decode stage.
        Instruction instruction = currentInstructionSource().read();
        DecodedInst decoded(instruction);
        decoded.location(currentInstructionSource().memoryAddress());

        Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_INSTRUCTIONS, decoded);

        if (DEBUG) {
          static const string names[] = {"FIFO", "cache", "unknown"};
          cout << this->name() << " selected instruction from "
               << names[currentPacket.location.component] << ": " << decoded << endl;
        }

        // Make sure we didn't read a junk instruction. "nor r0, r0, r0 -> 0" seems
        // pretty unlikely to ever come up in a real program.
        assert((instruction.toInt() != 0) && "Probable junk instruction");

        if (decoded.endOfIPK()) {
          // Check for the special case of single-instruction persistent packets.
          // These do not need to be read repeatedly, so remove the persistent flag.
          if (currentPacket.persistent &&
              currentPacket.memAddr == currentInstructionSource().memoryAddress()) {
            decoded.persistent(true);
            currentPacket.persistent = false;
          }

          // If a fetch in a persistent packet is sure to execute, the packet
          // should no longer be persistent.
          // FIXME: this is a temporary stop-gap measure until fetches have
          // moved to their proper location in the DecodeStage. This pipeline
          // stage will then receive notification of a new fetch before reading
          // a new instruction, so this block won't be needed.
          if (currentPacket.persistent) {
            switch (decoded.opcode()) {
              case InstructionMap::OP_FETCH:
              case InstructionMap::OP_FETCHR:
              case InstructionMap::OP_FETCHPST:
              case InstructionMap::OP_FETCHPSTR:
              case InstructionMap::OP_FILL:
              case InstructionMap::OP_FILLR:
              case InstructionMap::OP_PSEL_FETCH:
              case InstructionMap::OP_PSEL_FETCHR:
                currentPacket.persistent = false;
                break;
              default:
                break;
            }
          }

          if (currentPacket.persistent)
            currentInstructionSource().startNewPacket(currentPacket.location.index);
          else {
            currentPacket.reset();
            readState = RS_READY;
          }
        }

        outputInstruction(decoded);
        instructionCompleted();

        next_trigger(clock.negedge_event());
      }
      break;
    }
  } // end switch

}

// Continually ensure that any fetched instruction packets are available. Read
// requests from the fetchBuffer, and update the pending packet with the
// location to access locally. Only ever have a single fetch in progress.
void FetchStage::writeLoop() {

  switch (writeState) {
    case WS_READY: {
      if (fetchBuffer.empty()) {
        // Wait for a fetch request to arrive.
        next_trigger(fetchBuffer.writeEvent());
        break;
      }
      else if (!roomToFetch()) {
        // Wait for there to be space in the cache to fetch a new packet.
        next_trigger(cache.fillChangedEvent());
        break;
      }
      else if (pendingPacket.active() && pendingPacket.execute) {
        // The pending packet is where we store all the information about the
        // packet we're fetching. Wait for it to become available.
        next_trigger(clock.posedge_event());
        break;
      }
      else if (!iOutputBufferReady.read()) {
        next_trigger(iOutputBufferReady.posedge_event());
        break;
      }
      else {
        writeState = WS_CHECK_TAGS;
        // Fall through to next state.
      }
    }
    /* no break */

    case WS_CHECK_TAGS: {
      FetchInfo fetch = fetchBuffer.peek();
      pendingPacket.reset();
      bool cached = inCache(fetch.address, fetch.operation);

      if (cached) {
        // If the packet is already cached, we need to do nothing and can serve
        // the next fetch request.
        fetchBuffer.read();
        writeState = WS_READY;
        next_trigger(clock.posedge_event());
      }
      else {
        // Send fetch request.
        MemoryRequest request(MemoryRequest::IPK_READ, fetch.address);

        // Select the bank to access based on the memory address.
        uint increment = core()->channelMapTable[0].computeAddressIncrement(fetch.address);
        ChannelID destination = fetch.destination.addPosition(increment);

        NetworkData flit(request, destination);
        flit.setReturnAddr(fetch.returnAddress);
        assert(!oFetchRequest.valid());
        oFetchRequest.write(flit);

        if (fetch.returnAddress == 0)
          pendingPacket.location.component = IPKFIFO;
        else
          pendingPacket.location.component = IPKCACHE;

        if (!currentPacket.active())
          Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_INSTRUCTIONS, DecodedInst());

        // Instructions are received automatically by the subcomponents. Wait
        // for them to signal that the packet has finished arriving.
        writeState = WS_READY;
        next_trigger(packetArrivedEvent);
      }

      break;
    }
  } // end switch

}

void FetchStage::updateReady() {
  // Consider the pipeline to be stalled if the first pipeline stage is not
  // allowed to do any work. Only report the stall status when it changes.

  if (canSendInstruction() == stalled) {
    stalled = !canSendInstruction();
    core()->pipelineStalled(stalled);
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

MemoryAddr FetchStage::getInstAddress() const {
  // Hack - I know that accessing the memory address doesn't change anything.
  const InstructionStore& source =
      const_cast<FetchStage*>(this)->currentInstructionSource();
  return source.memoryAddress();
}

bool FetchStage::canCheckTags() const {
  // Can only prepare for another packet if there is a space to store its
  // information.
  return !fetchBuffer.full();
}

void FetchStage::checkTags(MemoryAddr addr,
                           opcode_t operation,
                           ChannelID channel,
                           ChannelIndex returnChannel) {
  // Note: we don't actually check the tags here. We just queue up the request
  // and check the tags at the next opportunity.

  FetchInfo fetch(addr, operation, channel, returnChannel);
  fetchBuffer.write(fetch);

  // Break out of persistent mode if we have another packet to execute.
  currentPacket.persistent = false;
}

bool FetchStage::inCache(const MemoryAddr addr, opcode_t operation) {
  if (DEBUG)
    cout << this->name() << " looking up tag 0x" << std::hex << addr << std::dec << ": ";

  // I suppose the address could technically be 0, but this always indicates an
  // error with the current arrangement.
  assert(addr > 0);

  PacketInfo& packet = pendingPacket;

  // Find out where, if anywhere, the packet is located.
  packet.location   = lookup(addr);

  packet.memAddr    = addr;
  packet.inCache    = packet.location.index != NOT_IN_CACHE;
  packet.execute    = operation != InstructionMap::OP_FILL &&
                      operation != InstructionMap::OP_FILLR;
  packet.persistent = operation == InstructionMap::OP_FETCHPST ||
                      operation == InstructionMap::OP_FETCHPSTR;
  newPacketAvailable.notify();

  bool found = packet.inCache;

  if (DEBUG)
    cout << (found ? "" : "not ") << "cached" << endl;

  // Clear the packet information if we aren't going to do anything with it.
  // This may be the case if we are prefilling the cache with instructions.
  if (packet.inCache && !packet.execute)
    packet.reset();

  Instrumentation::IPKCache::tagCheck(id, packet.inCache, addr, previousFetch);

  previousFetch = addr;

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
    cout << this->name() << " jumped by " << offset << " instructions" << endl;

  currentInstructionSource().jump(offset);

  // In case the packet has ended since reading the jump instruction. There
  // will be no effect otherwise.
  readState = RS_READ;
  newPacketAvailable.notify();
}

void FetchStage::nextIPK() {
  currentPacket.persistent = false;

  // Stop the current packet executing if it hasn't finished arriving yet.
  if (currentPacket.arriving())
    currentPacket.execute = false;

  if (currentPacket.location.component != UNKNOWN)
    currentInstructionSource().cancelPacket();

  core()->nextIPK();
}

MemoryAddr FetchStage::newPacketArriving(const InstLocation& location) {
  // Check whether we fetched this packet, or whether it's being sent to us
  // from another core.
  // FIXME: it's not possible to be sure - should we add in extra restrictions?
  // My simple approximation is to check whether a fetch has been issued.
  bool fetched = !fetchBuffer.empty();

  // If we fetched it, and we know that it is due to execute, set it as the
  // pending packet.
  if (fetched) {
    FetchInfo lastFetch = fetchBuffer.read(); // Pop from queue.

    bool willExecute = (lastFetch.operation != InstructionMap::OP_FILL)
                    || (lastFetch.operation != InstructionMap::OP_FILLR);

    if (willExecute) {
      pendingPacket.location = location;
      pendingPacket.memAddr = lastFetch.address;
      pendingPacket.persistent = (lastFetch.operation == InstructionMap::OP_FETCHPST)
                              || (lastFetch.operation == InstructionMap::OP_FETCHPSTR);
      newPacketAvailable.notify();
    }

    return lastFetch.address;
  }
  // Otherwise, only set it as the pending packet if nothing else is waiting.
  else {
    if (!pendingPacket.active()) {
      pendingPacket.location = location;
      pendingPacket.memAddr = 0;
      pendingPacket.persistent = false;
      newPacketAvailable.notify();

      return pendingPacket.memAddr;
    }
    else
      return 0;
  }
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

  packetArrivedEvent.notify();
}

InstructionStore& FetchStage::currentInstructionSource() {
  assert(currentPacket.location.component != UNKNOWN);

  if (currentPacket.location.component == IPKFIFO)
    return fifo;
  else if (currentPacket.location.component == IPKCACHE)
    return cache;
  else
    throw InvalidOptionException("instruction source component",
                                 currentPacket.location.component);
}

void FetchStage::getNextInstruction() {
  // This pipeline stage is dedicated to getting instructions, so call the main
  // method from here.
  readLoop();
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
    fifo("IPKfifo"),
    fetchBuffer(1, "fetchBuffer") {

  oFlowControl.init(2);
  oDataConsumed.init(2);

  readState = RS_READY;
  writeState = WS_READY;

  stalled             = false;  // Start off idle, but not stalled.
  previousFetch       = DEFAULT_TAG;

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

  // readLoop is called from another method.
  SC_METHOD(writeLoop);

  SC_METHOD(updateReady);
  sensitive << /*canSendEvent() << FIXME*/ instructionCompletedEvent;
  // do initialise

}
