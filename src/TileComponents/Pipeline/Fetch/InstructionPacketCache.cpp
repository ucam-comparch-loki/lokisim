/*
 * InstructionPacketcache->cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketCache.h"
#include "FetchStage.h"
#include "../../../Memory/IPKCacheDirectMapped.h"
#include "../../../Memory/IPKCacheFullyAssociative.h"
#include "../../../Utility/Instrumentation.h"
#include "../../../Utility/Instrumentation/IPKCache.h"
#include "../../../Utility/Instrumentation/Stalls.h"

/* Initialise the contents of the cache with a list of Instructions. */
void InstructionPacketCache::storeCode(const std::vector<Instruction>& instructions) {
  assert(instructions.size() > 0);
  assert(instructions.size() <= cache->size());

  CacheIndex writePos = cache->storeCode(instructions);

  if (finishedPacketWrite) {
    InstLocation location;
    location.component = IPKCACHE;
    location.index = writePos;
    parent()->newPacketArriving(location);
  }
  finishedPacketWrite = instructions.back().endOfPacket();

  if (finishedPacketWrite)
    parent()->packetFinishedArriving();

  // SystemC segfaults if we notify an event before simulation has started.
  if (sc_core::sc_start_of_simulation_invoked())
    cacheFillChanged.notify();
}

const Instruction InstructionPacketCache::read() {
  if (ENERGY_TRACE)
    Instrumentation::IPKCache::read(id);

  Instruction inst = cache->read();
  cacheFillChanged.notify();

  return inst;
}

void InstructionPacketCache::write(const Instruction inst) {
  if (ENERGY_TRACE)
    Instrumentation::IPKCache::write(id);
  if (DEBUG)
    cout << this->name() << " received Instruction: " << inst << endl;

  CacheIndex writePos = cache->write(inst);
  cacheFillChanged.notify();

  // Do some bookkeeping if we have finished an instruction packet or are
  // starting a new one.
  if (finishedPacketWrite) {
    InstLocation location;
    location.component = IPKCACHE;
    location.index = writePos;
    MemoryAddr tag = parent()->newPacketArriving(location);
    cache->setTag(tag);
  }

  finishedPacketWrite = inst.endOfPacket();
  if (finishedPacketWrite)
    parent()->packetFinishedArriving();
}

CacheIndex InstructionPacketCache::lookup(MemoryAddr tag) {
  // Shouldn't check tags more than once in a clock cycle.
  assert(timeLastChecked != Instrumentation::currentCycle());
  timeLastChecked = Instrumentation::currentCycle();

  CacheIndex position = cache->lookup(tag);
  bool inCache = (position != NOT_IN_CACHE);

  // It is possible that looking up a tag will result in us immediately jumping
  // to a new instruction, which would change how full the cache is.
  if (inCache)
    cacheFillChanged.notify();
  else
    cacheMissEvent.notify();

  return position;
}

bool InstructionPacketCache::packetExists(CacheIndex position) const {
  return cache->packetExists(position);
}

void InstructionPacketCache::startNewPacket(CacheIndex position) {
  cache->setReadPointer(position);
}

/* Jump to a new instruction specified by the offset amount. */
void InstructionPacketCache::jump(JumpOffset offset) {
  cache->jump(offset/BYTES_PER_WORD);
  cacheFillChanged.notify();
}

void InstructionPacketCache::cancelPacket() {
  cache->cancelPacket();
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
void InstructionPacketCache::updateFlowControl() {
  flowControl.write(!cache->full());
}

void InstructionPacketCache::dataConsumedAction() {
  dataConsumed.write(!dataConsumed.read());
}

/* Returns whether or not the cache is empty. */
bool InstructionPacketCache::isEmpty() const {
  return cache->empty();
}

bool InstructionPacketCache::isFull() const {
  return cache->full();
}

bool InstructionPacketCache::roomToFetch() const {
  return cache->canFetch();
}

FetchStage* InstructionPacketCache::parent() const {
  return static_cast<FetchStage*>(this->get_parent());
}

/* Constructors and destructors */
InstructionPacketCache::InstructionPacketCache(sc_module_name name, const ComponentID& ID) :
    Component(name, ID) {

  cache = new IPKCacheFullyAssociative(IPK_CACHE_SIZE, IPK_CACHE_TAGS, string(this->name()));
//  cache = new IPKCacheDirectMapped(IPK_CACHE_SIZE, string(this->name()));

  timeLastChecked = -1;

  // As soon as the first instruction arrives, queue it up to execute.
  finishedPacketWrite = true;

  SC_METHOD(receivedInst);
  sensitive << instructionIn;
  dont_initialize();

  SC_METHOD(updateFlowControl);
  sensitive << cacheFillChanged;
  // do initialise

  SC_METHOD(dataConsumedAction);
  sensitive << cache->dataConsumedEvent();
  dont_initialize();

}

InstructionPacketCache::~InstructionPacketCache() {
  delete cache;
}
