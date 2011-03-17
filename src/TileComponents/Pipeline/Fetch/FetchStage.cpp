/*
 * FetchStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "FetchStage.h"
#include "../../Cluster.h"
#include "../../../Datatype/DecodedInst.h"

double FetchStage::area() const {
  return cache.area() + fifo.area();
}

double FetchStage::energy() const {
  return cache.energy() + fifo.energy();
}

void FetchStage::newCycle() {
  // Would like this to be its own SC_METHOD, but that causes problems with
  // virtual inheritance for some reason.

  if(toIPKFIFO.event()) {
    Instruction instToFIFO = static_cast<Instruction>(toIPKFIFO.read());
    fifo.write(instToFIFO);
  }
  if(toIPKCache.event()) {
    Instruction instToCache = static_cast<Instruction>(toIPKCache.read());
    cache.write(instToCache);
  }
}

void FetchStage::cycleSecondHalf() {

  calculateSelect();

  if(readyIn.read()) {
    // Select a new instruction if the decode stage is ready for one, unless
    // the FIFO and cache are both empty.
    if(!cache.isEmpty() || !fifo.isEmpty()) {
      MemoryAddr instAddr;

      if(usingCache) {
        lastInstruction = cache.read();
        instAddr = cache.getInstAddress();
      }
      else {
        lastInstruction = fifo.read();
        // We don't know the address this instruction came from, so make
        // something up which would never happen.
        instAddr = 0xFFFFFFFF;
      }

      DecodedInst decoded(lastInstruction);
      decoded.location(instAddr);

      dataOut.write(decoded);
      idle.write(false);

      if(DEBUG) {
        printf("%s selected instruction from %s: ", this->name(),
                                                    usingCache?"cache":"FIFO");
        cout << lastInstruction << endl;
      }
    }
    else idle.write(true);  // Idle if we have no instructions.
  }
  else idle.write(true);    // Idle if we can't send any instructions.
}

void FetchStage::updateReady() {
  // Consider the pipeline to be stalled if the first pipeline stage is not
  // allowed to do any work. Only report the stall status when it changes.
  while(true) {
    wait(readyIn.default_event());

    if(readyIn.read() == stalled) {
      parent()->pipelineStalled(!readyIn.read());
      stalled = !readyIn.read();
    }
  }
}

void FetchStage::initialise() {
  idle.write(cache.isEmpty());
}

void FetchStage::storeCode(const std::vector<Instruction>& instructions) {
  cache.storeCode(instructions);
}

MemoryAddr FetchStage::getInstIndex() const {
  return cache.getInstAddress();
}

bool FetchStage::inCache(const MemoryAddr a) {
  return cache.lookup(a);
}

bool FetchStage::roomToFetch() const {
  return cache.roomToFetch();
}

/* Perform any status updates required when we receive a position to jump to. */
void FetchStage::jump(const JumpOffset offset) {
  usingCache = true;
  cache.jump(offset);
}

void FetchStage::setPersistent(bool persistent) {
  cache.updatePersistent(persistent);
}

void FetchStage::updatePacketAddress(const MemoryAddr addr) const {
  parent()->updateCurrentPacket(addr);
}

void FetchStage::refetch() const {
  parent()->refetch();
}

/* Choose whether to take an instruction from the cache or FIFO next.
 * Use FIFO if already executing a packet from FIFO,
 *          or cache finished a packet and FIFO has instructions.
 * Use cache if already executing a packet from cache,
 *          or FIFO is empty and cache has instructions. */
void FetchStage::calculateSelect() {
  if(usingCache) {
    justFinishedPacket = lastInstruction.endOfPacket();
    usingCache = (fifo.isEmpty() || !justFinishedPacket) && !cache.isEmpty();
  }
  else {
    usingCache = fifo.isEmpty();
  }
}

FetchStage::FetchStage(sc_module_name name, ComponentID ID) :
    PipelineStage(name, ID),
    StageWithSuccessor(name, ID),
    cache("IPKcache"),
    fifo("IPKfifo") {

  id = ID;

  stalled     = false;  // Start off idle, but not stalled.
  usingCache  = true;
  flowControl = new sc_out<int>[2];

  // Connect FIFO and cache to network
  fifo.flowControl(flowControl[0]);
  cache.flowControl(flowControl[1]);

}

FetchStage::~FetchStage() {
  delete[] flowControl;
}
