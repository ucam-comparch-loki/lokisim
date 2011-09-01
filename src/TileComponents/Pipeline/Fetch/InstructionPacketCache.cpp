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
  assert(instructions.size() > 0);
  assert(instructions.size() <= cache.size());

  cache.storeCode(instructions);

  // SystemC segfaults if we notify an event before simulation has started.
  if(Instrumentation::currentCycle() != 0) cacheFillChanged.notify();
}

Instruction InstructionPacketCache::read() {
  Instruction inst = cache.read();
  Instrumentation::l0Read(id);

  if(finishedPacketRead) startOfPacketTasks();
  if(inst.endOfPacket()) endOfPacketTasks();

  cacheFillChanged.notify();

  return inst;
}

void InstructionPacketCache::write(const Instruction inst) {
  MemoryAddr addr;

  Instrumentation::l0Write(id);
  if(DEBUG) cout << this->name() << " received Instruction: " << inst << endl;

  if((nextIPKAddress != 0xFFFFFFFF) && finishedPacketWrite) {
    // Only associate the tag with the first instruction of the packet.
    // Means that if a packet is searched for, execution starts at the start.
    addr = nextIPKAddress;

    // We have now "consumed" the address, so it is no longer valid.
    nextIPKAddress = 0xFFFFFFFF;
  }
  else {
    addr = 0xFFFFFFFF;
    // Deal with remote fills in here -- received packet, but have no address
  }

  try {
    // The write method throws an exception if the packet queued up to execute
    // next is being overwritten.
    cache.write(addr, inst);
  }
  catch(std::exception& e) {
    nextIPKAddress = pendingPacket;
    refetch(nextIPKAddress);
  }

  // Make a note for next cycle if it will be the start of a new packet.
  finishedPacketWrite = inst.endOfPacket();

  cacheFillChanged.notify();
}

/* See if an instruction packet is in the cache, and if so, prepare to
 * execute it. */
bool InstructionPacketCache::lookup(const MemoryAddr addr, operation_t operation) {
  if(DEBUG) cout << this->name() << " looking up tag " << addr << ": ";

  // Shouldn't check tags more than once in a clock cycle.
  assert(timeLastChecked != Instrumentation::currentCycle());
  timeLastChecked = Instrumentation::currentCycle();

  bool inCache = cache.checkTags(addr, operation);
  if(DEBUG) cout << (inCache ? "" : "not ") << "in cache" << endl;

  if(inCache) {
    // Store the address in case we need it later - there is the possibility
    // that the packet we have just found in the cache may yet be overwritten.
    pendingPacket = addr;

    // It is possible that looking up a tag will result in us immediately jumping
    // to a new instruction, which would change how full the cache is.
    cacheFillChanged.notify();
  }
  else {
    // If we don't have the instructions, we will receive them soon, so store
    // the address to use as the tag.
    nextIPKAddress = addr;
    cacheMissEvent.notify();
  }

  Instrumentation::l0TagCheck(id, inCache);

  return inCache;

}

/* Jump to a new instruction specified by the offset amount. */
void InstructionPacketCache::jump(const JumpOffset offset) {
  if(DEBUG) cout << this->name() << ": ";   // cache prints the rest

  cache.jump(offset/BYTES_PER_INSTRUCTION);

  cacheFillChanged.notify();
}

const sc_core::sc_event& InstructionPacketCache::fillChangedEvent() const {
  return cacheFillChanged;
}

void InstructionPacketCache::receivedInst() {
  // Need to cast from Word to Instruction.
  Instruction inst = static_cast<Instruction>(instructionIn.read());
  write(inst);
}

/* Update the signal saying whether there is enough room to fetch another
 * packet. */
void InstructionPacketCache::sendCredit() {
  flowControl.write(!cache.full());
}

void InstructionPacketCache::updatePacketAddress(const MemoryAddr addr) const {
  parent()->updatePacketAddress(addr);
}

void InstructionPacketCache::refetch(const MemoryAddr addr) const {
  parent()->refetch(addr);
}

MemoryAddr InstructionPacketCache::getInstAddress() const {
  return cache.getInstLocation();
}

/* Returns whether or not the cache is empty. */
bool InstructionPacketCache::isEmpty() const {
  return cache.empty();
}

bool InstructionPacketCache::roomToFetch() const {
  return cache.canFetch();
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
  // Currently nothing to do. Remove this method?
  finishedPacketRead = false;
}

FetchStage* InstructionPacketCache::parent() const {
  return static_cast<FetchStage*>(this->get_parent());
}

/* Constructors and destructors */
InstructionPacketCache::InstructionPacketCache(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    cache(IPK_CACHE_SIZE, string(name)) {

  nextIPKAddress = 0xFFFFFFFF;
  finishedPacketRead = true;
  finishedPacketWrite = true;
  timeLastChecked = -1;

  SC_METHOD(receivedInst);
  sensitive << instructionIn;
  dont_initialize();

  SC_METHOD(sendCredit);
  sensitive << cacheFillChanged;
  // do initialise

}
