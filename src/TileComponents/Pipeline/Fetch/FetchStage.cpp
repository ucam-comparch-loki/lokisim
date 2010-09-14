/*
 * FetchStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "FetchStage.h"

double FetchStage::area() const {
  return cache.area() + fifo.area();// + mux.area();
}

double FetchStage::energy() const {
  return cache.energy() + fifo.energy();// + mux.energy();
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
    if(readyIn.read()) {
      if(!(fifo.isEmpty() && cache.isEmpty())) {
        if(usingCache) instruction.write(cache.read());
        else           instruction.write(fifo.read());
        idle.write(false);
      }
      else idle.write(true);
    }
  }
}

void FetchStage::newFIFOInst() {
  Instruction instToFIFO = static_cast<Instruction>(toIPKFIFO.read());
  fifo.write(instToFIFO);
}

void FetchStage::newCacheInst() {
  Instruction instToCache = static_cast<Instruction>(toIPKCache.read());
  cache.write(instToCache);
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
    usingCache = (fifo.isEmpty() || !justFinishedPacket) && !cache.isEmpty();
    result = CACHE;
  }
  else {
    usingCache = fifo.isEmpty();
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
void FetchStage::jump(int8_t offset) {
  usingCache = true;
  cache.jump(offset);
}

FetchStage::FetchStage(sc_module_name name) :
    PipelineStage(name),
    cache("IPKcache"),
    fifo("IPKfifo") {

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

  // Connect FIFO and cache to network
  fifo.flowControl(flowControl[0]);
  cache.flowControl(flowControl[1]);

}

FetchStage::~FetchStage() {

}
