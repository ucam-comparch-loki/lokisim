/*
 * FetchStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "FetchStage.h"

double FetchStage::area() const {
  return cache.area() + fifo.area() + mux.area();
}

double FetchStage::energy() const {
  return cache.energy() + fifo.energy() + mux.energy();
}

void FetchStage::storeCode(std::vector<Instruction>& instructions) {
  cache.storeCode(instructions);
}

int FetchStage::getInstIndex() const {
  return cache.getInstIndex();
}

void FetchStage::newCycle() {
  while(true) {
    wait(clock.posedge_event());

    // Select a new instruction unless the processor is stalled, or the FIFO
    // and cache are both empty.
    if(!stall.read()) {
      if(!(fifoEmpty.read() && cache.isEmpty())) {
        if(usingCache) readFromCache.write(!readFromCache.read());
        else readFromFIFO.write(!readFromFIFO.read());
        idle.write(false);
      }
      else idle.write(true);
    }
  }
}

void FetchStage::newFIFOInst() {
  Instruction instToFIFO = static_cast<Instruction>(toIPKFIFO.read());

  if(DEBUG) cout<<fifo.name()<<" received Instruction:  "<<instToFIFO<<endl;

  toFIFO.write(instToFIFO);
}

void FetchStage::newCacheInst() {
  Instruction instToCache = static_cast<Instruction>(toIPKCache.read());

  if(DEBUG) cout<<cache.name()<<" received Instruction: "<<instToCache<<endl;

  toCache.write(instToCache);
}

/* Choose whether to take an instruction from the cache or FIFO.
 * Use FIFO if already executing a packet from FIFO,
 *          or cache finished a packet and FIFO has instructions.
 * Use cache if already executing a packet from cache,
 *          or FIFO is empty and cache has instructions. */
short FetchStage::calculateSelect() {

  short result;

  if(usingCache) {
    Instruction i = cacheToMux.read();
    justFinishedPacket = i.endOfPacket();
    usingCache = (fifoEmpty.read() || !justFinishedPacket) && !cache.isEmpty();
    result = CACHE;
  }
  else {
    usingCache = fifoEmpty.read();
    result = usingCache ? CACHE : FIFO;
  }

  return result;

}

void FetchStage::select() {
  short sel = calculateSelect();

  if((sel==CACHE && cacheToMux.event()) || (sel==FIFO && FIFOtoMux.event())) {
    muxSelect.write(sel);

    if(DEBUG) {
      printf("%s selected instruction from %s: ", this->name(),
                                                  sel==CACHE?"cache":"FIFO");
      if(sel==FIFO) cout << FIFOtoMux.read() << endl;
      else          cout << cacheToMux.read() << endl;
    }
  }
}

/* Perform any status updates required when we receive a position to jump to. */
void FetchStage::jump() {
  usingCache = true;
  jumpOffsetSig.write(jumpOffset.read());
}

FetchStage::FetchStage(sc_module_name name) :
    PipelineStage(name),
    cache("IPKcache"),
    fifo("IPKfifo"),
    mux("fetchmux") {

  usingCache = true;
  flowControl = new sc_out<int>[2];

// Register methods
  SC_METHOD(newFIFOInst);
  sensitive << toIPKFIFO;
  dont_initialize();

  SC_METHOD(newCacheInst);
  sensitive << toIPKCache;
  dont_initialize();

  SC_METHOD(select);
  sensitive << cacheToMux << FIFOtoMux << clock.pos();
  dont_initialize();

  SC_METHOD(jump);
  sensitive << jumpOffset;
  dont_initialize();

// Connect everything up
  cache.clock(clock);
  fifo.clock(clock);

  cache.instructionIn(toCache);
  fifo.in(toFIFO);
  mux.select(muxSelect);

  cache.instructionOut(cacheToMux); mux.in1(cacheToMux);
  fifo.out(FIFOtoMux); mux.in2(FIFOtoMux);
  mux.result(instruction);

  cache.address(address);
  cache.jumpOffset(jumpOffsetSig);
  cache.cacheHit(cacheHit);
  cache.isRoomToFetch(roomToFetch);
  cache.refetch(refetch);
  cache.currentPacket(currentPacket);
  cache.readInstruction(readFromCache);
  cache.persistent(persistent);
  cache.flowControl(flowControl[1]);

  fifo.empty(fifoEmpty);
  fifo.readInstruction(readFromFIFO);
  fifo.flowControl(flowControl[0]);

}

FetchStage::~FetchStage() {

}
