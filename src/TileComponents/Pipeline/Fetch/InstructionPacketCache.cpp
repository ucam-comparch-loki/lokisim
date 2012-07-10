/*
 * InstructionPacketCache.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketCache.h"
#include "FetchStage.h"
#include "../../../Utility/Instrumentation.h"
#include "../../../Utility/Instrumentation/IPKCache.h"
#include "../../../Utility/Instrumentation/Stalls.h"

/* Initialise the contents of the cache with a list of Instructions. */
void InstructionPacketCache::storeCode(const std::vector<Instruction>& instructions) {
  assert(instructions.size() > 0);
  assert(instructions.size() <= cache.size());

  cache.storeCode(instructions);

  // SystemC segfaults if we notify an event before simulation has started.
  if (sc_core::sc_start_of_simulation_invoked())
    cacheFillChanged.notify();
}

Instruction InstructionPacketCache::read() {
  if (ENERGY_TRACE)
    Instrumentation::IPKCache::read(id);

  Instruction inst = cache.read();
  cacheFillChanged.notify();

  // If we the cache is now empty, but we are still waiting for instructions,
  // record this.
  if (cache.stalled())
    Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_INSTRUCTIONS);

  return inst;
}

void InstructionPacketCache::write(const Instruction inst) {
  // If the cache was blocked, it isn't any more.
  if (cache.stalled())
    Instrumentation::Stalls::unstall(id);

  if (ENERGY_TRACE)
    Instrumentation::IPKCache::write(id);
  if (DEBUG)
    cout << this->name() << " received Instruction: " << inst << endl;

  cache.write(inst);
  cacheFillChanged.notify();
}

/* See if an instruction packet is in the cache, and if so, prepare to
 * execute it. */
bool InstructionPacketCache::lookup(const MemoryAddr addr, opcode_t operation) {
  if (DEBUG)
    cout << this->name() << " looking up tag " << addr << ": ";

  // Shouldn't check tags more than once in a clock cycle.
  assert(timeLastChecked != Instrumentation::currentCycle());
  timeLastChecked = Instrumentation::currentCycle();

  bool inCache = cache.checkTags(addr, operation);
  if (DEBUG)
    cout << (inCache ? "" : "not ") << "in cache" << endl;

  // It is possible that looking up a tag will result in us immediately jumping
  // to a new instruction, which would change how full the cache is.
  if (inCache)
    cacheFillChanged.notify();
  else
    cacheMissEvent.notify();

  if (ENERGY_TRACE)
    Instrumentation::IPKCache::tagCheck(id, inCache);

  return inCache;
}

/* Jump to a new instruction specified by the offset amount. */
void InstructionPacketCache::jump(const JumpOffset offset) {
  cache.jump(offset/BYTES_PER_WORD);
  cacheFillChanged.notify();
}

void InstructionPacketCache::nextIPK() {
  cache.cancelPacket();
}

bool InstructionPacketCache::packetInProgress() const {
  return cache.packetInProgress();
}

const sc_event& InstructionPacketCache::fillChangedEvent() const {
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

FetchStage* InstructionPacketCache::parent() const {
  return static_cast<FetchStage*>(this->get_parent());
}

/* Constructors and destructors */
InstructionPacketCache::InstructionPacketCache(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    cache(IPK_CACHE_SIZE, string(this->name())) {

  timeLastChecked = -1;

  SC_METHOD(receivedInst);
  sensitive << instructionIn;
  dont_initialize();

  SC_METHOD(sendCredit);
  sensitive << cacheFillChanged;
  // do initialise

}
