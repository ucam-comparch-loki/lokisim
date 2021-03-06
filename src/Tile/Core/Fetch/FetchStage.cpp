/*
 * FetchStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "FetchStage.h"

#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/Assert.h"
#include "../../../Utility/Instrumentation/IPKCache.h"
#include "../../../Exceptions/InvalidOptionException.h"
#include "../../../Utility/Instrumentation/Network.h"
#include "../Core.h"

void FetchStage::execute() {
  // Do nothing - everything is handled in readLoop and writeLoop.
}

// Continually read instructions from the current instruction packet and pass
// them to the rest of the pipeline. When the packet completes, switch to the
// pending packet, when available.
void FetchStage::readLoop() {

  switch (readState) {
    case RS_READY: {
      // Any instructions waiting in the FIFO have priority over the cache.
      if (fifoPendingPacket.active() && fifoPendingPacket.execute)
        switchToPacket(fifoPendingPacket);
      else if (currentPacket.persistent) {
        currentInstructionSource().startNewPacket(currentPacket.location.index);

        static const string names[] = {"FIFO", "cache", "unknown"};
        LOKI_LOG(3) << this->name() << " restarted persistent packet: " << names[currentPacket.location.component] <<
          ", position " << currentPacket.location.index << " (" << LOKI_HEX(currentPacket.memAddr) << ")" << endl;

        readState = RS_READ;
        next_trigger(sc_core::SC_ZERO_TIME);
      }
      else if (cachePendingPacket.active() && cachePendingPacket.execute)
        switchToPacket(cachePendingPacket);
      // Nothing to do - wait until a new instruction packet arrives.
      else
        next_trigger(newPacketAvailable);

      break;
    }

    // TODO: nextIPK needs to break us out of this state
    case RS_READ: {
      if (currentInstructionSource().isEmpty()) {
        Instrumentation::Stalls::stall(id(), Instrumentation::Stalls::STALL_INSTRUCTIONS, DecodedInst());
        next_trigger(currentInstructionSource().writeEvent());
      }
      else if (nextStageBlocked()) {
        next_trigger(nextStageUnblockedEvent());
      }
      else if (!clock.negedge()) {
        next_trigger(clock.negedge_event());
      }
      else {
        // The Instruction becomes a DecodedInst here to simplify various interfaces
        // throughout the pipeline. The decoding actually happens in the decode stage.
        Instruction instruction = currentInstructionSource().read();
        currentInst = DecodedInst(instruction);
        currentInst.location(getCurrentAddress());
        currentInst.source(currentPacket.location.component);

        Instrumentation::Stalls::unstall(id(), Instrumentation::Stalls::STALL_INSTRUCTIONS, currentInst);

        static const string names[] = {"FIFO", "cache", "unknown"};
        LOKI_LOG(2) << this->name() << " selected instruction from "
             << names[currentPacket.location.component] << ": " << currentInst << endl;

        // Make sure we didn't read a junk instruction. "nor r0, r0, r0 -> 0" seems
        // pretty unlikely to ever come up in a real program.
        loki_assert_with_message(instruction.toInt() != 0, "Probable junk instruction from address 0x%x", currentInst.location());

        if (currentInst.endOfIPK()) {
          // Check for the special case of single-instruction persistent packets.
          // These do not need to be read repeatedly, so remove the persistent flag.
          if (currentPacket.persistent &&
              currentPacket.memAddr == currentInstructionSource().memoryAddress()) {
            currentInst.persistent(true);
            currentPacket.persistent = false;
          }

          readState = RS_READY;
          next_trigger(sc_core::SC_ZERO_TIME);
        }
        else
          next_trigger(clock.negedge_event());

        outputInstruction(currentInst);
        instructionCompleted();
      }
      break;
    }
  } // end switch

}

void FetchStage::switchToPacket(PacketInfo& packet) {
  if (packet.location.index == NOT_IN_CACHE) {
    Instrumentation::Stalls::stall(id(), Instrumentation::Stalls::STALL_INSTRUCTIONS, DecodedInst());
    if (packet.location.component == IPKFIFO)
      next_trigger(fifo.writeEvent());
    else
      next_trigger(cache.writeEvent());
  }
  else {
    currentPacket = packet;

    currentInstructionSource().startNewPacket(currentPacket.location.index);

    static const string names[] = {"FIFO", "cache", "unknown"};
    LOKI_LOG(2) << this->name() << " switched to pending packet: " << names[currentPacket.location.component] <<
      ", position " << currentPacket.location.index << " (0x" << std::hex << currentPacket.memAddr << std::dec << ")" << endl;

    packet.reset();
    readState = RS_READ;
    next_trigger(sc_core::SC_ZERO_TIME);
  }
}

// Continually ensure that any fetched instruction packets are available. Read
// requests from the fetchBuffer, and update the pending packet with the
// location to access locally. Only ever have a single fetch in progress.
void FetchStage::writeLoop() {

  switch (writeState) {
    case WS_READY: {
      // Wait for a fetch request to arrive.
      if (fetchBuffer.empty()) {
        next_trigger(fetchBuffer.writeEvent());
      }
      // Wait for there to be space in the cache to fetch a new packet.
      else if (!roomToFetch()) {
        next_trigger(cache.readEvent() | fifo.readEvent());
      }
      // The pending packet is where we store all the information about the
      // packet we're fetching. Wait for it to become available.
      else if (fifoPendingPacket.active() || cachePendingPacket.active()) {
        next_trigger(clock.posedge_event());
      }
      else if (!iOutputBufferReady.read()) {
        next_trigger(iOutputBufferReady.posedge_event());
      }
      else {
        writeState = WS_FETCH;
        next_trigger(sc_core::SC_ZERO_TIME);
      }
      break;
    }

    case WS_FETCH: {
      activeFetch = fetchBuffer.read();

      if (activeFetch.networkInfo.returnChannel == 0)
        fifoPendingPacket.reset();
      else
        cachePendingPacket.reset();

      bool cached = inCache(activeFetch);

      if (cached) {
        // If the packet is already cached, we need to do nothing and can serve
        // the next fetch request.
        writeState = WS_READY;
        next_trigger(clock.posedge_event());
      }
      else {
        writeState = WS_RECEIVE;
        sendRequest(activeFetch);

        if (activeFetch.networkInfo.returnChannel == 0)
          next_trigger(fifo.writeEvent());
        else
          next_trigger(cache.writeEvent());
      }

      break;
    }

    case WS_RECEIVE: {
      if (activeFetch.complete) {
        writeState = WS_READY;
        next_trigger(clock.posedge_event());
      }
      else {
        // If this is the last instruction in the cache line, we need to send a
        // new request, possibly to a different memory bank.
        // Cache line = 32 bytes = 2^5 bytes
        if ((activeFetch.address & 0x1F) == 0) {
          writeState = WS_CONTINUE;
          next_trigger(sc_core::SC_ZERO_TIME);
        }
        else {
          // Wait for the next instruction to arrive.
          if (activeFetch.networkInfo.returnChannel == 0)
            next_trigger(fifo.writeEvent());
          else
            next_trigger(cache.writeEvent());
        }
      }

      break;
    }

    // Very similar to WS_READY, but since we are continuing a packet, we can
    // leave out some of the checks.
    case WS_CONTINUE: {
      // Wait for there to be space in the cache to fetch a new line.
      if (!cache.roomToFetch()) {
        next_trigger(cache.readEvent());
      }
      else if (!iOutputBufferReady.read()) {
        next_trigger(iOutputBufferReady.posedge_event());
      }
      else {
        LOKI_LOG(3) << this->name() << " requesting IPK continuation from "
                 << LOKI_HEX(activeFetch.address) << endl;
        sendRequest(activeFetch);
        writeState = WS_RECEIVE;

        if (activeFetch.networkInfo.returnChannel == 0)
          next_trigger(fifo.writeEvent());
        else
          next_trigger(cache.writeEvent());
      }
      break;
    }
  } // end switch

}

void FetchStage::sendRequest(const FetchInfo& fetch) {
  if (MAGIC_MEMORY) {
    ChannelID returnAddress(id(), fetch.networkInfo.returnChannel);
    core().magicMemoryAccess(IPK_READ, fetch.address, returnAddress);
  }
  else {
    NetworkData flit;

    if (fetch.networkInfo.isMemory) {
      // Select the bank to access based on the memory address.
      uint increment = core().channelMapTable[0].computeAddressIncrement(fetch.address);
      ChannelID destination(id().tile.x, id().tile.y, fetch.networkInfo.bank + increment + core().coresThisTile(), fetch.networkInfo.channel);
      flit = NetworkData(fetch.address, destination, fetch.networkInfo, IPK_READ, true);
    }
    else {
      ChannelID destination(id().tile.x, id().tile.y, fetch.networkInfo.bank, fetch.networkInfo.channel);
      flit = NetworkData(fetch.address, destination, false, false, true);
    }

    loki_assert(!oFetchRequest.valid());
    oFetchRequest.write(flit);
  }

  // If we don't already have a packet to execute, we are now stalled
  // waiting for this one to arrive.
  if (!currentPacket.active())
    Instrumentation::Stalls::stall(id(), Instrumentation::Stalls::STALL_INSTRUCTIONS, DecodedInst());
}

void FetchStage::updateReady() {
  // Consider the pipeline to be stalled if the first pipeline stage is not
  // allowed to do any work. Only report the stall status when it changes.

  if (nextStageBlocked() != stalled) {
    stalled = nextStageBlocked();
    core().pipelineStalled(stalled);
  }
}

void FetchStage::storeCode(const std::vector<Instruction>& instructions) {
  loki_assert(!cachePendingPacket.inCache);

  cachePendingPacket.memAddr = 0;
  cachePendingPacket.location.component = IPKCACHE;

  cache.storeCode(instructions);
}

void FetchStage::deliverInstructionInternal(const NetworkData& flit) {
  Instruction inst = static_cast<Instruction>(flit.payload());

  switch (flit.channelID().channel) {
    case 0:
      fifo.write(inst);
      break;
    case 1:
      cache.write(inst);
      break;
    default:
      throw InvalidOptionException("instruction channel", flit.channelID().channel);
      break;
  }
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
                           EncodedCMTEntry netInfo) {
  // Note: we don't actually check the tags here. We just queue up the request
  // and check the tags at the next opportunity.
  FetchInfo fetch(addr, operation, ChannelMapEntry::memoryView(netInfo));
  fetchBuffer.write(fetch);

  // Break out of persistent mode if we have another packet to execute.
  if (currentPacket.persistent) {
    currentPacket.persistent = false;

    // If the previous instruction was also the end of the packet, abort the
    // next iteration.
    if (currentInst.endOfIPK())
      readState = RS_READY;
  }
}

bool FetchStage::inCache(const FetchInfo& fetch) {
  LOKI_LOG(2) << this->name() << " looking up tag " << LOKI_HEX(fetch.address) << ": ";

  // I suppose the address could technically be 0, but this always indicates an
  // error with the current arrangement.
  loki_assert_with_message(fetch.address > 0, "Address = 0x%x", fetch.address);

  opcode_t operation = fetch.operation;
  InstLocation location = lookup(fetch.address);

  // If we do not currently have the packet, we know which input it will arrive
  // on.
  if (location.component == UNKNOWN) {
    ChannelIndex returnChannel = fetch.networkInfo.returnChannel;
    location.component = (returnChannel == 0) ? IPKFIFO : IPKCACHE;
  }

  PacketInfo& packet = (location.component == IPKFIFO)
                     ? fifoPendingPacket
                     : cachePendingPacket;

  packet.location   = location;
  packet.memAddr    = fetch.address;
  packet.inCache    = packet.location.index != NOT_IN_CACHE;
  packet.execute    = operation != ISA::OP_FILL &&
                      operation != ISA::OP_FILLR;
  packet.persistent = operation == ISA::OP_FETCHPST ||
                      operation == ISA::OP_FETCHPSTR;
  newPacketAvailable.notify();

  bool found = packet.inCache;

  LOKI_LOG(2) << (found ? "" : "not ") << "cached" << endl;

  // Clear the packet information if we aren't going to do anything with it.
  // This may be the case if we are prefilling the cache with instructions.
  if (packet.inCache && !packet.execute)
    packet.reset();

  Instrumentation::IPKCache::tagCheck(core(), packet.inCache, fetch.address, previousFetch);

  previousFetch = fetch.address;

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
  LOKI_LOG(2) << this->name() << " jumped by " << offset << " instructions" << endl;

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

  core().nextIPK();
}

void FetchStage::fifoInstructionArrived(Instruction inst) {
  // Increment the fetch address so we can fetch a new line when necessary.
  if (activeFetch.networkInfo.returnChannel == 0) {
    activeFetch.address += BYTES_PER_WORD;
    activeFetch.complete = inst.endOfPacket();
  }
}

void FetchStage::cacheInstructionArrived(Instruction inst) {
  // Increment the fetch address so we can fetch a new line when necessary.
  if (activeFetch.networkInfo.returnChannel == 1) {
    activeFetch.address += BYTES_PER_WORD;
    activeFetch.complete = inst.endOfPacket();
  }
}

MemoryAddr FetchStage::newPacketArriving(const InstLocation& location) {
  PacketInfo& pendingPacket = (location.component == IPKFIFO)
                            ? fifoPendingPacket
                            : cachePendingPacket;

  // We now know where to read the packet from.
  pendingPacket.location = location;

  // If the packet didn't already have a tag, it is probably arriving from
  // another core.
  if (pendingPacket.memAddr == DEFAULT_TAG)
    pendingPacket.memAddr = 0;

  newPacketAvailable.notify(sc_core::SC_ZERO_TIME);
  return pendingPacket.memAddr;
}

void FetchStage::packetFinishedArriving(InstructionSource source) {
  // Look for the first packet in the queue which we don't have.
  PacketInfo* packet;
  if (currentPacket.active() && !currentPacket.inCache && currentPacket.location.component == source)
    packet = &currentPacket;
  else if (source == IPKFIFO)
    packet = &fifoPendingPacket;
  else
    packet = &cachePendingPacket;

  packet->inCache = true;

  // If we are not due to execute this packet, clear its entry in the queue so
  // we can fetch another.
  if (!packet->execute)
    packet->reset();

  packetArrivedEvent.notify(sc_core::SC_ZERO_TIME);
}

InstructionStore& FetchStage::currentInstructionSource() {
  loki_assert(currentPacket.location.component != UNKNOWN);

  if (currentPacket.location.component == IPKFIFO)
    return fifo;
  else if (currentPacket.location.component == IPKCACHE)
    return cache;
  else
    throw InvalidOptionException("instruction source component",
                                 currentPacket.location.component);
}

MemoryAddr FetchStage::getCurrentAddress() const {
  // To allow context switching, if this packet is from the FIFO, assume it
  // is an interrupt, and attach some information about the packet which
  // would have been fetched from the cache, had the interrupt not arrived.
  if (currentPacket.location.component == IPKFIFO) {
    if (cachePendingPacket.active()) {
      // Ensure that the address of the cache packet ends up in the program
      // counter, and set the lowest bit if the fetch was to be persistent.
      MemoryAddr address = cachePendingPacket.memAddr;
      if (cachePendingPacket.persistent)
        address |= 1;
      return address;
    }
    else {
      // If there is no cache packet, use some obviously junk address.
      return 0xffffffff;
    }
  }
  else {
    return const_cast<FetchStage*>(this)->currentInstructionSource().memoryAddress();
  }
}

void FetchStage::prepareNextInstruction() {
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
  if (cache.isFull())
    os << cache.name() << " is full." << endl;
  if (fifo.isFull())
    os << fifo.name() << " is full." << endl;
}

FetchStage::FetchStage(sc_module_name name,
                       const fifo_parameters_t& fifoParams,
                       const cache_parameters_t& cacheParams) :
    FirstPipelineStage(name),
    iToCache("iCacheInstruction"),
    iToFIFO("iFIFOInstruction"),
    oFetchRequest("oFetchRequest"),
    iOutputBufferReady("iOutputBufferReady"),
    cache("IPKcache", cacheParams),
    fifo("IPKfifo", fifoParams),
    fetchBuffer("fetchBuffer", 1) {

  // Connect ports.
  fifo.clock(clock);
  cache.clock(clock);
  iToFIFO(fifo);
  iToCache(cache);

  readState = RS_READY;
  writeState = WS_READY;

  stalled             = false;  // Start off idle, but not stalled.
  previousFetch       = DEFAULT_TAG;
  activeFetch         = FetchInfo();

  currentPacket.reset(); fifoPendingPacket.reset(); cachePendingPacket.reset();

  // readLoop is called from another method.
  SC_METHOD(writeLoop);

  SC_METHOD(updateReady);
  sensitive << instructionCompletedEvent;
  // do initialise

}
