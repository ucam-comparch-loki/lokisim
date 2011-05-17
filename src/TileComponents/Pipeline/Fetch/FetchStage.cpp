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

void FetchStage::execute() {
  // Loop forever, executing the appropriate tasks each clock cycle.
  while(true) {
    wait(clock.negedge_event());
    cycleSecondHalf();
  }
}

void FetchStage::cycleSecondHalf() {
  // Select a new instruction if the decode stage is ready for one, unless
  // the FIFO and cache are both empty.
  if(readyIn.read() && (!cache.isEmpty() || !fifo.isEmpty())) {
    calculateSelect();
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

void FetchStage::storeCode(const std::vector<Instruction>& instructions) {
  cache.storeCode(instructions);
  idle.write(false);
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

FetchStage::FetchStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    StageWithSuccessor(name, ID),
    cache("IPKcache", ID),
    fifo("IPKfifo") {

  stalled     = false;  // Start off idle, but not stalled.
  usingCache  = true;
  flowControl = new sc_out<bool>[2];

  // Connect FIFO and cache to network
  fifo.clock(clock);
  fifo.instructionIn(toIPKFIFO);
  fifo.flowControl(flowControl[0]);
  cache.clock(clock);
  cache.instructionIn(toIPKCache);
  cache.flowControl(flowControl[1]);

  idle.initialize(true);

}

FetchStage::~FetchStage() {
  delete[] flowControl;
}
