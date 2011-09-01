/*
 * FetchStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "FetchStage.h"
#include "../../Cluster.h"
#include "../../../Datatype/DecodedInst.h"

void FetchStage::execute() {
  bool isIdle;

  if(cache.isEmpty() && fifo.isEmpty()) { // Wait for an instruction
    isIdle = true;
    next_trigger(fifo.fillChangedEvent() | cache.fillChangedEvent());
  }
  else if(!readyIn.read()) {              // Wait until decoder is ready
    isIdle = false;
    next_trigger(readyIn.posedge_event());
  }
  else if(!clock.negedge()) {             // Do fetch at negedge (why?)
    isIdle = false;
    next_trigger(clock.negedge_event());
  }
  else {                                  // Pass an instruction to pipeline
    isIdle = false;
    getInstruction();
    next_trigger(clock.negedge_event());
  }

  // Update the idle signal if there was a change.
  if(idle.read() != isIdle) idle.write(isIdle);
}

void FetchStage::getInstruction() {
  // Select a new instruction if the decode stage is ready for one, unless
  // the FIFO and cache are both empty.

  assert(readyIn.read());

  calculateSelect();
  MemoryAddr instAddr;

  if(usingCache) {
    if(cache.isEmpty()) {
      next_trigger(cache.fillChangedEvent());
      return;
    }

    lastInstruction = cache.read();
    instAddr = cache.getInstAddress();
  }
  else {
    assert(!fifo.isEmpty());
//    if(fifo.isEmpty()) {
//      next_trigger(fifo.fillChangedEvent());
//      return;
//    }

    lastInstruction = fifo.read();
    // We don't know the address this instruction came from, so make
    // something up which would never happen.
    instAddr = 0xFFFFFFFF;
  }

  // The instruction becomes a "DecodedInst" here to simplify various interfaces
  // throughout the pipeline. The decoding happens in the decode stage.
  DecodedInst decoded(lastInstruction);
  decoded.location(instAddr);

  dataOut.write(decoded);

  if(DEBUG) {
    printf("%s selected instruction from %s: ", this->name(),
                                                usingCache?"cache":"FIFO");
    cout << lastInstruction << endl;
  }
}

void FetchStage::updateReady() {
  // Consider the pipeline to be stalled if the first pipeline stage is not
  // allowed to do any work. Only report the stall status when it changes.

  if(readyIn.read() == stalled) {
    parent()->pipelineStalled(!readyIn.read());
    stalled = !readyIn.read();
  }

  next_trigger(readyIn.default_event());
}

void FetchStage::storeCode(const std::vector<Instruction>& instructions) {
  cache.storeCode(instructions);
  idle.write(false);
}

MemoryAddr FetchStage::getInstIndex() const {
  return cache.getInstAddress();
}

bool FetchStage::inCache(const MemoryAddr addr, operation_t operation) {
  return cache.lookup(addr, operation);
}

bool FetchStage::roomToFetch() const {
  return cache.roomToFetch();
}

/* Perform any status updates required when we receive a position to jump to. */
void FetchStage::jump(const JumpOffset offset) {
  usingCache = true;
  cache.jump(offset);
}

void FetchStage::updatePacketAddress(const MemoryAddr addr) const {
  parent()->updateCurrentPacket(addr);
}

void FetchStage::refetch(const MemoryAddr addr) const {
  parent()->refetch(addr);
}

/* Choose whether to take an instruction from the cache or FIFO next.
 * Use FIFO if already executing a packet from FIFO,
 *          or cache finished a packet and FIFO has instructions.
 * Use cache if already executing a packet from cache,
 *          or FIFO is empty and cache has instructions. */
void FetchStage::calculateSelect() {
  if(usingCache) {
    justFinishedPacket = lastInstruction.endOfPacket();
    usingCache = (fifo.isEmpty() || !justFinishedPacket);// && !cache.isEmpty();
  }
  else {
    usingCache = fifo.isEmpty();
  }
}

FetchStage::FetchStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    cache("IPKcache", ID),
    fifo("IPKfifo") {

  stalled     = false;  // Start off idle, but not stalled.
  usingCache  = true;
  flowControl = new sc_out<bool>[2];

  // The last instruction "executed" was the last in its packet, so the first
  // instruction that arrives (through either input) is the one to execute.
  lastInstruction = Instruction(0);
  lastInstruction.predicate(Instruction::END_OF_PACKET);

  // Connect FIFO and cache to network
  fifo.clock(clock);
  fifo.instructionIn(toIPKFIFO);
  fifo.flowControl(flowControl[0]);
  cache.clock(clock);
  cache.instructionIn(toIPKCache);
  cache.flowControl(flowControl[1]);

  idle.initialize(true);

  SC_METHOD(execute);
  // do initialise

}

FetchStage::~FetchStage() {
  delete[] flowControl;
}
