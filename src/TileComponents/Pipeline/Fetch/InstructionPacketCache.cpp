/*
 * InstructionPacketCache.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketCache.h"

/* Initialise the contents of the cache with a list of Instructions */
void InstructionPacketCache::storeCode(std::vector<Instruction>& instructions) {
  cache.storeCode(instructions);

  instToSend = cache.read();

  if(instToSend.endOfPacket()) {
    startOfPacket = true;
  }
  else startOfPacket = false;

  if(instToSend.endOfPacket()) cache.switchToPendingPacket();
  writeNotify3.write(!writeNotify3.read()); // Invoke the write() method
  sentNewInst = true;
  outputWasRead = false;
}

/* Put a received instruction into the cache at the appropriate position */
void InstructionPacketCache::insertInstruction() {

  bool empty = cache.isEmpty();
  Address addr;

  try {
    // Only associate the tag with the first instruction of the packet.
    // Means that if a packet is searched for, execution starts at the start.
    addr = startOfPacket ? addresses.read() : Address(0,0);
  }
  catch(std::exception e) {
    addr = Address(0,0);
  }

  // Do we want to tag all instructions, or only the first one of each packet?
  cache.write(addr, in.read());
  if(in.read().endOfPacket()) {
    startOfPacket = true;
  }
  else startOfPacket = false;

  if(empty && outputWasRead) {                // Send the instruction immediately
    instToSend = cache.read();

    if(instToSend.endOfPacket()) cache.switchToPendingPacket();
    writeNotify1.write(!writeNotify1.read()); // Invoke the write() method
    sentNewInst = true;
    outputWasRead = false;
  }

}

/* See if an instruction packet is in the cache, and if so, prepare to execute it */
void InstructionPacketCache::lookup() {
  std::cout << "Looked up: " << address.read() << std::endl;
  bool inCache = cache.checkTags(address.read());
  cacheHit.write(inCache);

  // If we don't have the instructions, we will probably receive them soon
  if(!inCache) {
    if(!addresses.isFull()) addresses.write(address.read());
    else std::cout << "Wrote to full buffer in IPK cache." << std::endl;
  }
}

/* An instruction was read from the cache, so change to another packet if
 * necessary, and prepare the next instruction */
void InstructionPacketCache::finishedRead() {

  try {
    if(!sentNewInst && !cache.isEmpty()) {
      instToSend = cache.read();
      if(instToSend.endOfPacket()) cache.switchToPendingPacket();
      writeNotify2.write(!writeNotify2.read());
    }
  }
  catch(std::exception e) {     // There are no more instructions
    // Do nothing
  }

  sentNewInst = false;          // Reset for next cycle
  outputWasRead = true;

}

/* Update the signal saying whether there is enough room to fetch another packet */
void InstructionPacketCache::updateRTF() {
  isRoomToFetch.write(cache.remainingSpace() >= MAX_IPK_SIZE);
  flowControl.write(!cache.isFull());
}

/* Send the chosen instruction. We need a separate method for this because both
 * insertInstruction and finishedRead can result in the sending of new
 * instructions, but only one method is allowed to drive a particular wire. */
void InstructionPacketCache::write() {
  out.write(instToSend);
//  std::cout << "FetchStage sent Instruction: " << instToSend << std::endl;
}

/* Constructors and destructors */
InstructionPacketCache::InstructionPacketCache(sc_core::sc_module_name name) :
    Component(name),
    cache(IPK_CACHE_SIZE),
    addresses(4) {      // 4 = max outstanding fetches allowed

  sentNewInst = false;
  outputWasRead = true;   // Allow the first received instruction to pass through
  startOfPacket = true;

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

  SC_METHOD(write);
  sensitive << writeNotify1 << writeNotify2 << writeNotify3;
  dont_initialize();

}

InstructionPacketCache::~InstructionPacketCache() {

}
