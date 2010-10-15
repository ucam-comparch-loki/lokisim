/*
 * FetchStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "FetchStage.h"
#include "../../Cluster.h"

double FetchStage::area() const {
  return cache.area() + fifo.area();// + mux.area();
}

double FetchStage::energy() const {
  return cache.energy() + fifo.energy();// + mux.energy();
}

void FetchStage::newCycle() {

  // See if we have received any new instructions.
  checkInputs();

  calculateSelect();

}

void FetchStage::cycleSecondHalf() {
  // Consider the pipeline to be stalled if the first pipeline stage is not
  // allowed to do any work. Only report the stall status when it changes.
  if(readyIn.read() == stalled) {
    parent()->pipelineStalled(!readyIn.read());
    stalled = !readyIn.read();
  }

  if(readyIn.read()) {
    // Select a new instruction if the decode stage is ready for one, unless
    // the FIFO and cache are both empty.
    if(!(fifo.isEmpty() && cache.isEmpty())) {
      if(usingCache) lastInstruction = cache.read();
      else           lastInstruction = fifo.read();
      instruction.write(lastInstruction);
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

void FetchStage::initialise() {
  idle.write(cache.isEmpty());
}

void FetchStage::storeCode(std::vector<Instruction>& instructions) {
  cache.storeCode(instructions);
}

int FetchStage::getInstIndex() const {
  return cache.getInstIndex();
}

bool FetchStage::inCache(Address a) {
  return cache.lookup(a);
}

bool FetchStage::roomToFetch() const {
  return cache.roomToFetch();
}

/* Perform any status updates required when we receive a position to jump to. */
void FetchStage::jump(int16_t offset) {
  usingCache = true;
  cache.jump(offset);
}

void FetchStage::setPersistent(bool persistent) {
  cache.updatePersistent(persistent);
}

void FetchStage::updatePacketAddress(Address addr) {
  parent()->updateCurrentPacket(addr);
}

void FetchStage::refetch() {
  parent()->refetch();
}

void FetchStage::checkInputs() {
  if(toIPKFIFO.event()) {
    Instruction instToFIFO = static_cast<Instruction>(toIPKFIFO.read());
    fifo.write(instToFIFO);
  }
  if(toIPKCache.event()) {
    Instruction instToCache = static_cast<Instruction>(toIPKCache.read());
    cache.write(instToCache);
  }
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

FetchStage::FetchStage(sc_module_name name, uint16_t ID) :
    PipelineStage(name),
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

}
