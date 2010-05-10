/*
 * InstructionPacketCache.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketCache.h"

/* Initialise the contents of the cache with a list of Instructions. */
void InstructionPacketCache::storeCode(std::vector<Instruction>& instructions) {

  if(instructions.size() == 0) {
    cerr << "Error: loaded 0 instructions into " << this->name() << endl;
    return;
  }

  cache.storeCode(instructions);

  instToSend = cache.read();

  if(instToSend.endOfPacket()) endOfPacketTasks();

  wake(readyToWrite);   // Invoke the write() method
  sentNewInst = true;
  outputWasRead = false;
}

/* Put a received instruction into the cache at the appropriate position. */
void InstructionPacketCache::insertInstruction() {

  // TODO: stall pipeline until instructions arrive

  bool empty = cache.isEmpty();
  Address addr;

  if(!addresses.isEmpty() && startOfPacket) {
    // Only associate the tag with the first instruction of the packet.
    // Means that if a packet is searched for, execution starts at the start.
    addr = addresses.read();
  }
  else {
    addr = Address(0,0);
  }

  cache.write(addr, instructionIn.read());

  // Make a note for next cycle if it will be the start of a new packet.
  startOfPacket = instructionIn.read().endOfPacket();

  if(empty && outputWasRead) {                // Send the instruction immediately
    instToSend = cache.read();

    if(instToSend.endOfPacket()) endOfPacketTasks();
    else if(finishedPacketRead)  wake(startingPacket);

    wake(readyToWrite);   // Invoke the write() method
    sentNewInst = true;
    outputWasRead = false;
  }

}

/* See if an instruction packet is in the cache, and if so, prepare to
 * execute it. */
void InstructionPacketCache::lookup() {
  if(DEBUG) cout<<this->name()<<" looked up tag: "<<address.read()<<endl;
  bool inCache = cache.checkTags(address.read());
  cacheHit.write(inCache);

  // If we don't have the instructions, we will probably receive them soon,
  // so store the address to use as the tag.
  if(!inCache) {
    if(!addresses.isFull()) addresses.write(address.read());
      // Stall if the buffer is now full?
    else cerr << "Wrote to full buffer in " << this->name() << endl;
  }
}

/* An instruction was read from the cache, so change to another packet if
 * necessary, and prepare the next instruction. */
void InstructionPacketCache::finishedRead() {

  if(!sentNewInst && !cache.isEmpty()) {
    instToSend = cache.read();

    if(instToSend.endOfPacket()) endOfPacketTasks();
    else if(finishedPacketRead) {
      wake(startingPacket);
    }

    wake(readyToWrite);   // Invoke the write() method
  }

  sentNewInst = false;          // Reset for next cycle
  outputWasRead = true;

}

/* Jump to a new instruction specified by the offset amount. */
void InstructionPacketCache::jump() {
  if(DEBUG) cout << this->name() << ": ";   // cache prints the rest

  cache.jump(jumpOffset.read());
  instToSend = cache.read();
  if(instToSend.endOfPacket()) endOfPacketTasks();

  wake(readyToWrite);   // Invoke the write() method
}

/* Update the signal saying whether there is enough room to fetch another
 * packet. */
void InstructionPacketCache::updateRTF() {
  isRoomToFetch.write(cache.remainingSpace() >= MAX_IPK_SIZE);
  flowControl.write(!cache.isFull());
}

void InstructionPacketCache::updatePacketAddress(Address addr) {
  currentPacket.write(addr);
}

/* Send the chosen instruction. We need a separate method for this because both
 * insertInstruction and finishedRead can result in the sending of new
 * instructions, but only one method is allowed to drive a particular wire. */
void InstructionPacketCache::write() {
  instructionOut.write(instToSend);
//  cout << "FetchStage sent Instruction: " << instToSend << endl;
}

/* Convenience method, avoid using if possible. */
bool InstructionPacketCache::isEmpty() {
  return cache.isEmpty();
}

/* Perform any necessary tasks when the end of an instruction packet has been
 * reached. */
void InstructionPacketCache::endOfPacketTasks() {
  cache.switchToPendingPacket();
  finishedPacketRead = true;
}

/* Perform any necessary tasks when starting to read a new instruction packet. */
void InstructionPacketCache::startOfPacketTasks() {
  updatePacketAddress(cache.packetAddress());
  finishedPacketRead = false;
}

/* Constructors and destructors */
InstructionPacketCache::InstructionPacketCache(sc_module_name name) :
    Component(name),
    cache(IPK_CACHE_SIZE),
    addresses(4) {        // 4 = max outstanding fetches allowed

  sentNewInst = false;
  outputWasRead = true;   // Allow the first received instruction to pass through
  startOfPacket = true;

// Register methods
  SC_METHOD(insertInstruction);
  sensitive << instructionIn;
  dont_initialize();

  SC_METHOD(lookup);
  sensitive << address;
  dont_initialize();

  SC_METHOD(jump);
  sensitive << jumpOffset;
  dont_initialize();

  SC_METHOD(finishedRead);
  sensitive << readInstruction;
  dont_initialize();

  SC_METHOD(updateRTF);
  sensitive << clock.pos();
  // Do initialise

  SC_METHOD(startOfPacketTasks);
  sensitive << startingPacket;
  dont_initialize();

  SC_METHOD(write);
  sensitive << readyToWrite;
  dont_initialize();

}

InstructionPacketCache::~InstructionPacketCache() {

}
