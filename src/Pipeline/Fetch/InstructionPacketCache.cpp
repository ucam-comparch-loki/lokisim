/*
 * InstructionPacketCache.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketCache.h"

/* Put a received instruction into the cache at the appropriate position */
void InstructionPacketCache::insertInstruction() {

  // Do we want to tag all instructions, or only the first one of each packet?
  cache.write(addresses.peek(), in.read());
  if(in.read().endOfPacket()) addresses.pop();

  if(empty) {          // Send the data on immediately
    Instruction next = cache.read();
    if(next.endOfPacket()) cache.switchToPendingPacket();
    out.write(next);
    empty = false;
  }

}

/* See if an instruction packet is in the cache, and if so, prepare to execute it */
void InstructionPacketCache::lookup() {
  bool inCache = cache.checkTags(address.read());
  cacheHit.write(inCache);
  if(!inCache) addresses.write(address.read());
}

/* An instruction was read from the cache, so change to another packet if
 * necessary, and prepare the next instruction */
void InstructionPacketCache::finishedRead() {

  try {
    Instruction next = cache.read();
    if(next.endOfPacket()) cache.switchToPendingPacket();
    out.write(next);
  }
  catch(std::exception e) {   // There are no more instructions
    // nop => do nothing if this instruction is used
    // eop => switch to IPK FIFO if it has any pending instructions
    out.write(*(new Instruction("nop.eop")));
    empty = true;
  }

}

/* Update the signal saying whether there is enough room to fetch another packet */
void InstructionPacketCache::updateRTF() {
  isRoomToFetch.write(cache.remainingSpace() >= MAX_IPK_SIZE);
}

/* Constructors and destructors */
InstructionPacketCache::InstructionPacketCache(sc_core::sc_module_name name, int ID) :
    Component(name, ID),
    cache(IPK_CACHE_SIZE),
    addresses(4) {      // 4 = max outstanding fetches allowed

// Register methods
  SC_METHOD(insertInstruction);
  sensitive << in;
  dont_initialize();

  SC_METHOD(lookup);
  sensitive << address;
  dont_initialize();

  SC_METHOD(finishedRead);
  sensitive << readInstruction;
  dont_initialize();

  SC_METHOD(updateRTF);
  sensitive << clock.pos();
  // Do initialise

}

InstructionPacketCache::~InstructionPacketCache() {

}
