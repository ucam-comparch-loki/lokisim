/*
 * InstructionPacketCache.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketCache.h"
#include "FetchStage.h"
#include "../../../Exceptions/WritingToFullException.h"

/* Initialise the contents of the cache with a list of Instructions. */
void InstructionPacketCache::storeCode(const std::vector<Instruction>& instructions) {

  if(instructions.size() == 0) {
    cerr << "Error: loaded 0 instructions into " << this->name() << endl;
    return;
  }

  cache.storeCode(instructions);

}

Instruction InstructionPacketCache::read() {
  Instruction inst = cache.read();
  Instrumentation::l0Read(id);

  if(finishedPacketRead) startOfPacketTasks();
  if(inst.endOfPacket()) endOfPacketTasks();

  if(cache.justReadNewInst()) sendCredit();

  return inst;
}

void InstructionPacketCache::write(const Instruction inst) {
  MemoryAddr addr;

  if(!addresses.empty() && finishedPacketWrite) {
    // Only associate the tag with the first instruction of the packet.
    // Means that if a packet is searched for, execution starts at the start.
    addr = addresses.read();
  }
  else {
    addr = 0xFFFFFFFF;
    // Deal with remote fills in here -- received packet, but have no address
  }

  try {
    cache.write(addr, inst);
  }
  catch(std::exception& e) {
    // The write method throws an exception if the next packet needs to be
    // fetched again.
    if(!addresses.full()) addresses.write(pendingPacket);
    refetch();
  }

  // Make a note for next cycle if it will be the start of a new packet.
  finishedPacketWrite = inst.endOfPacket();

  Instrumentation::l0Write(id);
  if(DEBUG) cout << this->name() << " received Instruction: " << inst << endl;

}

/* See if an instruction packet is in the cache, and if so, prepare to
 * execute it. */
bool InstructionPacketCache::lookup(const MemoryAddr addr) {

  bool inCache = cache.checkTags(addr);

  if(DEBUG) cout << this->name() << " looked up tag " << addr << ": "
                 << (inCache ? "" : "not ") << "in cache" << endl;

  // If we don't have the instructions, we will receive them soon, so store
  // the address to use as the tag.
  if(!inCache) {
    if(!addresses.full()) addresses.write(addr);
      // Stall if the buffer is now full?
    else throw WritingToFullException(this->name());
  }
  else {
    // Store the address in case we need it later.
    pendingPacket = addr;
  }

  Instrumentation::l0TagCheck(id, inCache);

  return inCache;

}

/* Jump to a new instruction specified by the offset amount. */
void InstructionPacketCache::jump(const JumpOffset offset) {
  if(DEBUG) cout << this->name() << ": ";   // cache prints the rest

  cache.jump(offset/BYTES_PER_INSTRUCTION);
}

void InstructionPacketCache::updatePersistent(bool persistent) {
  cache.setPersistent(persistent);
}

/* Update the signal saying whether there is enough room to fetch another
 * packet. */
void InstructionPacketCache::sendCredit() {
  flowControl.write(1);
}

void InstructionPacketCache::updatePacketAddress(const MemoryAddr addr) const {
  parent()->updatePacketAddress(addr);
}

void InstructionPacketCache::refetch() const {
  parent()->refetch();
}

MemoryAddr InstructionPacketCache::getInstAddress() const {
  return cache.getInstLocation();
}

/* Returns whether or not the cache is empty. */
bool InstructionPacketCache::isEmpty() const {
  return cache.empty();
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
  // Wait a cycle to update packet address?
  updatePacketAddress(cache.packetAddress());
  finishedPacketRead = false;
}

FetchStage* InstructionPacketCache::parent() const {
  return dynamic_cast<FetchStage*>(this->get_parent());
}

/* Constructors and destructors */
InstructionPacketCache::InstructionPacketCache(sc_module_name name) :
    Component(name),
    cache(IPK_CACHE_SIZE, string(name)),
    addresses(4, string(name)) {  // 4 = max outstanding fetches allowed

  id = parent()->id;

  finishedPacketRead = true;
  finishedPacketWrite = true;

}

InstructionPacketCache::~InstructionPacketCache() {

}
