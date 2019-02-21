/*
 * InstructionPacketcache->cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "InstructionPacketCache.h"

#include "../../../Memory/IPKCacheDirectMapped.h"
#include "../../../Memory/IPKCacheFullyAssociative.h"
#include "FetchStage.h"
#include "../../../Utility/Assert.h"
#include "../../../Utility/Instrumentation.h"
#include "../../../Utility/Instrumentation/IPKCache.h"
#include "../../../Utility/Instrumentation/Stalls.h"

/* Initialise the contents of the cache with a list of Instructions. */
void InstructionPacketCache::storeCode(const std::vector<Instruction>& instructions) {
  loki_assert(instructions.size() > 0);
  loki_assert_with_message(instructions.size() <= cache->size(),
      "Trying to store %d instructions", instructions.size());

  CacheIndex writePos = cache->storeCode(instructions);

  for (CacheIndex j=0, i=writePos; j<instructions.size(); j++) {
    addresses[i] = 0;
    i++;
    if (i >= cache->size())
      i = 0;
  }

  if (finishedPacketWrite) {
    InstLocation location;
    location.component = IPKCACHE;
    location.index = writePos;
    parent().newPacketArriving(location);
  }
  finishedPacketWrite = instructions.back().endOfPacket();

  if (finishedPacketWrite)
    parent().packetFinishedArriving(IPKCACHE);

  // SystemC segfaults if we notify an event before simulation has started.
  if (sc_core::sc_start_of_simulation_invoked())
    cacheWrite.notify(sc_core::SC_ZERO_TIME);
}

const Instruction InstructionPacketCache::read() {
  Instrumentation::IPKCache::read(parent().core());

  Instruction inst = cache->read();
  cacheRead.notify(sc_core::SC_ZERO_TIME);

  CacheIndex readPointer = cache->getReadPointer();
  lastReadAddr = addresses[readPointer];

  return inst;
}

void InstructionPacketCache::write(const Instruction inst) {
  if (ENERGY_TRACE)
    Instrumentation::IPKCache::write(parent().core());
  LOKI_LOG << this->name() << " received Instruction: " << inst << endl;

  CacheIndex writePos = cache->write(inst);
  cacheWrite.notify(sc_core::SC_ZERO_TIME);

  // Do some bookkeeping if we have finished an instruction packet or are
  // starting a new one.
  if (finishedPacketWrite) {
    InstLocation location;
    location.component = IPKCACHE;
    location.index = writePos;
    MemoryAddr tag = parent().newPacketArriving(location);
    cache->setTag(tag);
    lastWriteAddr = tag;
  }
  else
    lastWriteAddr += BYTES_PER_WORD;

  addresses[writePos] = lastWriteAddr;

  finishedPacketWrite = inst.endOfPacket();
  if (finishedPacketWrite)
    parent().packetFinishedArriving(IPKCACHE);
}

CacheIndex InstructionPacketCache::lookup(MemoryAddr tag) {
  // Shouldn't check tags more than once in a clock cycle.
  loki_assert(timeLastChecked != Instrumentation::currentCycle());
  timeLastChecked = Instrumentation::currentCycle();

  CacheIndex position = cache->lookup(tag);
  bool inCache = (position != NOT_IN_CACHE);

  // It is possible that looking up a tag will result in us immediately jumping
  // to a new instruction, which would change how full the cache is.
  if (inCache)
    cacheWrite.notify(sc_core::SC_ZERO_TIME);
  else
    cacheMissEvent.notify(sc_core::SC_ZERO_TIME);

  return position;
}

MemoryAddr InstructionPacketCache::memoryAddress() const {
  return lastReadAddr;
}

void InstructionPacketCache::startNewPacket(CacheIndex position) {
  cache->setReadPointer(position);
}

/* Jump to a new instruction specified by the offset amount. */
void InstructionPacketCache::jump(JumpOffset offset) {
  cache->jump(offset);
  cacheWrite.notify(sc_core::SC_ZERO_TIME);  // Ensures that cache doesn't appear empty now.
}

void InstructionPacketCache::cancelPacket() {
  cache->cancelPacket();
}

const sc_event& InstructionPacketCache::readEvent() const {
  return cacheRead;
}

const sc_event& InstructionPacketCache::writeEvent() const {
  return cacheWrite;
}

/* Update the signal saying whether there is enough room to fetch another
 * packet. */
void InstructionPacketCache::updateFlowControl() {
  // Writes are expensive (in terms of simulation time), so only update when the
  // signal changes.
  bool canWrite = !cache->full();
  if (canWrite != oFlowControl.read())
    oFlowControl.write(canWrite);
}

void InstructionPacketCache::dataConsumedAction() {
  oDataConsumed.write(!oDataConsumed.read());
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

FetchStage& InstructionPacketCache::parent() const {
  return static_cast<FetchStage&>(*(this->get_parent_object()));
}

/* Constructors and destructors */
InstructionPacketCache::InstructionPacketCache(sc_module_name name,
                                               const cache_parameters_t& params) :
    LokiComponent(name),
    oFlowControl("oFlowControl"),
    oDataConsumed("oDataConsumed"),
    addresses(params.size, DEFAULT_TAG) {

  cache = new IPKCacheFullyAssociative(string(this->name()), params.size, params.numTags, params.maxIPKSize);
//  cache = new IPKCacheDirectMapped(string(this->name()), params.size, params.maxIPKSize);

  lastReadAddr = 0;
  lastWriteAddr = 0;

  timeLastChecked = -1;

  // As soon as the first instruction arrives, queue it up to execute.
  finishedPacketWrite = true;

  SC_METHOD(updateFlowControl);
  sensitive << cacheRead << cacheWrite;
  // do initialise

  SC_METHOD(dataConsumedAction);
  sensitive << cache->dataConsumedEvent();
  dont_initialize();

}

InstructionPacketCache::~InstructionPacketCache() {
  delete cache;
}
