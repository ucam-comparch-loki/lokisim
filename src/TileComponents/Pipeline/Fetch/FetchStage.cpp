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
  bool isIdle = false;

  if(cache.isEmpty() && fifo.isEmpty()) { // Wait for an instruction
    isIdle = true;
    next_trigger(fifo.fillChangedEvent() | cache.fillChangedEvent());
  }
  else if(!canSendInstruction()) {        // Wait until decoder is ready
    next_trigger(canSendEvent());
  }
  else if(!clock.negedge()) {             // Do fetch at negedge (why?)
    next_trigger(clock.negedge_event());
  }
  else {                                  // Pass an instruction to pipeline
    getInstruction();
    next_trigger(clock.negedge_event());
  }

  // Update the idle signal if there was a change.
  if(idle.read() != isIdle) idle.write(isIdle);
}

void FetchStage::getInstruction() {
  // Select a new instruction if the decode stage is ready for one, unless
  // the FIFO and cache are both empty.

  assert(canSendInstruction());

  calculateSelect();
  MemoryAddr instAddr;
  Instruction instruction;

  if(usingCache) {
    if(cache.isEmpty()) {
      next_trigger(cache.fillChangedEvent());
      return;
    }

    instruction = cache.read();
    instAddr = cache.getInstAddress();
  }
  else {
    assert(!fifo.isEmpty());
//    if(fifo.isEmpty()) {
//      next_trigger(fifo.fillChangedEvent());
//      return;
//    }

    instruction = fifo.read();
    // We don't know the address this instruction came from, so make
    // something up which would never happen.
    instAddr = 0xFFFFFFFF;
  }

  // The instruction becomes a "DecodedInst" here to simplify various interfaces
  // throughout the pipeline. The decoding happens in the decode stage.
  DecodedInst decoded(instruction);
  decoded.location(instAddr);

  outputInstruction(decoded);
  instructionCompleted();

  if(DEBUG) {
    printf("%s selected instruction from %s: ", this->name(),
                                                usingCache?"cache":"FIFO");
    cout << decoded << endl;
  }
}

void FetchStage::updateReady() {
  // Consider the pipeline to be stalled if the first pipeline stage is not
  // allowed to do any work. Only report the stall status when it changes.

  if(canSendInstruction() == stalled) {
    stalled = !canSendInstruction();
    parent()->pipelineStalled(stalled);
  }
}

void FetchStage::storeCode(const std::vector<Instruction>& instructions) {
  cache.storeCode(instructions);
  idle.write(false);
}

MemoryAddr FetchStage::getInstIndex() const {
  return cache.getInstAddress();
}

bool FetchStage::inCache(const MemoryAddr addr, opcode_t operation) {
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

/* Choose whether to take an instruction from the cache or FIFO next.
 * Use FIFO if already executing a packet from FIFO,
 *          or cache finished a packet and FIFO has instructions.
 * Use cache if already executing a packet from cache,
 *          or FIFO is empty and cache has instructions. */
void FetchStage::calculateSelect() {
  if(usingCache) {
    usingCache = (fifo.isEmpty() || cache.packetInProgress());// && !cache.isEmpty();
  }
  else {
    usingCache = fifo.isEmpty();
  }
}

void FetchStage::nextIPK() {
  cache.nextIPK();
  parent()->nextIPK();
}

void FetchStage::getNextInstruction() {
  // This pipeline stage is dedicated to getting instructions, so call the main
  // method from here.
  execute();
}

FetchStage::FetchStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    cache("IPKcache", ID),
    fifo("IPKfifo") {

  stalled     = false;  // Start off idle, but not stalled.
  usingCache  = true;
  flowControl.init(2);

  // Connect FIFO and cache to network
  fifo.clock(clock);
  fifo.instructionIn(toIPKFIFO);
  fifo.flowControl(flowControl[0]);
  cache.clock(clock);
  cache.instructionIn(toIPKCache);
  cache.flowControl(flowControl[1]);

  idle.initialize(true);

  SC_METHOD(updateReady);
  sensitive << /*canSendEvent() << FIXME*/ instructionCompletedEvent;
  // do initialise

}
