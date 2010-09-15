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

  Instruction inst = cache.read();

  if(inst.endOfPacket()) endOfPacketTasks();

}

Instruction InstructionPacketCache::read() {
  Instruction inst = cache.read();

  if(inst.endOfPacket())       endOfPacketTasks();
  else if(finishedPacketRead)  startOfPacketTasks();

  updateFlowControl();

  return inst;
}

void InstructionPacketCache::write(Instruction inst) {
  Address addr;

  if(!addresses.isEmpty() && startOfPacket) {
    // Only associate the tag with the first instruction of the packet.
    // Means that if a packet is searched for, execution starts at the start.
    addr = addresses.read();
  }
  else {
    addr = Address(0,0);
    // Deal with remote fills in here -- received packet, but have no address
  }

  try {
    cache.write(addr, inst);
  }
  catch(std::exception e) {
    // The write method throws an exception if the next packet needs to be
    // fetched again.
    if(!addresses.isFull()) addresses.write(pendingPacket);
//    refetch.write(!refetch.read());
  }

  // Make a note for next cycle if it will be the start of a new packet.
  startOfPacket = inst.endOfPacket();

  updateFlowControl();

  if(DEBUG) cout << this->name() << " received Instruction: " << inst << endl;

}

/* See if an instruction packet is in the cache, and if so, prepare to
 * execute it. */
bool InstructionPacketCache::lookup(Address addr) {

  bool inCache = cache.checkTags(addr);

  if(DEBUG) cout << this->name() << " looked up tag: " << addr << ": "
                 << (inCache ? "" : "not ") << "in cache" << endl;

  // If we don't have the instructions, we will receive them soon, so store
  // the address to use as the tag.
  if(!inCache) {
    if(!addresses.isFull()) addresses.write(addr);
      // Stall if the buffer is now full?
    else cerr << "Wrote to full buffer in " << this->name() << endl;
  }
  else {
    // Store the address in case we need it later.
    pendingPacket = addr;
  }

  Instrumentation::IPKCacheHit(inCache);

  return inCache;

}

/* Jump to a new instruction specified by the offset amount. */
void InstructionPacketCache::jump(int8_t offset) {
  if(DEBUG) cout << this->name() << ": ";   // cache prints the rest

  cache.jump(offset/BYTES_PER_WORD);
//  instToSend = cache.read();
//  if(instToSend.endOfPacket()) endOfPacketTasks();
//
//  wake(startingPacket);
//
//  wake(readyToWrite);     // Invoke the write() method
}

void InstructionPacketCache::updatePersistent(bool persistent) {
  cache.setPersistent(persistent);
}

/* Update the signal saying whether there is enough room to fetch another
 * packet. */
void InstructionPacketCache::updateFlowControl() {
//  isRoomToFetch.write((cache.remainingSpace() >= MAX_IPK_SIZE)
//                    || finishedPacketRead);

  flowControl.write(cache.remainingSpace());
}

//void InstructionPacketCache::updatePacketAddress(Address addr) {
//  currentPacket.write(addr);
//}

/* Send the chosen instruction. We need a separate method for this because both
 * insertInstruction and finishedRead can result in the sending of new
 * instructions, but only one method is allowed to drive a particular wire. */
//void InstructionPacketCache::write() {
//  instructionOut.write(instToSend);
//  lastInstSent = sc_core::sc_time_stamp().to_default_time_units();
//  outputWasRead = false;
//}

int InstructionPacketCache::getInstIndex() const {
  return cache.getInstIndex();
}

/* Returns whether or not the cache is empty. */
bool InstructionPacketCache::isEmpty() const {
  return cache.isEmpty();
}

bool InstructionPacketCache::roomToFetch() const {
  return cache.remainingSpace() >= MAX_IPK_SIZE;
}

/* Perform any necessary tasks when the end of an instruction packet has been
 * reached. */
void InstructionPacketCache::endOfPacketTasks() {
  if(DEBUG) cout << this->name() << " ";  // Cache will print rest
  cache.switchToPendingPacket();
  finishedPacketRead = true;
}

/* Perform any necessary tasks when starting to read a new instruction packet. */
void InstructionPacketCache::startOfPacketTasks() {
//  updatePacketAddress(cache.packetAddress());
  finishedPacketRead = false;
}

bool InstructionPacketCache::sentInstThisCycle() const {
  return sc_core::sc_time_stamp().to_default_time_units() == lastInstSent;
}

/* Constructors and destructors */
InstructionPacketCache::InstructionPacketCache(sc_module_name name) :
    Component(name),
    cache(IPK_CACHE_SIZE),
    addresses(4) {        // 4 = max outstanding fetches allowed

  outputWasRead = true;   // Allow the first received instruction to pass through
  startOfPacket = true;
  lastInstSent = -1;

}

InstructionPacketCache::~InstructionPacketCache() {

}
