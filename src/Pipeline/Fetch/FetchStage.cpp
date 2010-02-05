/*
 * FetchStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "FetchStage.h"

void FetchStage::newCycle() {

  selectVal = calculateSelect();

  if(selectVal==1) readFromFIFO.write(!readFromFIFO.read());
  else readFromCache.write(!readFromCache.read());

//  muxSelect.write(sel);

}

void FetchStage::newFIFOInst() {
  Instruction* instToFIFO = new Instruction(
                              static_cast<Instruction>(toIPKQueue.read()));

  toFIFO.write(*instToFIFO);
}

void FetchStage::newCacheInst() {
  Instruction* instToCache = new Instruction(
                              static_cast<Instruction>(toIPKCache.read()));

  toCache.write(*instToCache);
}

/* Choose whether to take an instruction from the cache or FIFO.
 * Use FIFO if already executing a packet from FIFO,
 *          or cache finished a packet and FIFO has instructions.
 * Use cache if already executing a packet from cache,
 *          or FIFO is empty and cache has instructions. */
short FetchStage::calculateSelect() {

  // 0 = cache, 1 = FIFO
  short result = usingCache ? 0 : 1;

  if(usingCache) {
    Instruction i = cacheToMux.read();
    usingCache = !i.endOfPacket() || fifoEmpty.read();
  }
  else {
    usingCache = fifoEmpty.read();
  }

  // Deal with situations where neither input has instructions?

  return result;

}


void FetchStage::select() {
  muxSelect.write(calculateSelect());
}

FetchStage::FetchStage(sc_core::sc_module_name name, int ID) :
    PipelineStage(name, ID),
    cache("IPKcache", ID),
    fifo("IPKfifo", ID),
    mux("fetchmux") {

  usingCache = true;
  selectVal = 0;

// Register methods
  SC_METHOD(newFIFOInst);
  sensitive << toIPKQueue;
  dont_initialize();

  SC_METHOD(newCacheInst);
  sensitive << toIPKCache;
  dont_initialize();

  SC_METHOD(select);
  sensitive << cacheToMux << FIFOtoMux;
  dont_initialize();

// Connect everything up
  cache.clock(clock);

  cache.in(toCache);
  fifo.in(toFIFO);
  mux.select(muxSelect);

  cache.out(cacheToMux); mux.in1(cacheToMux);
  fifo.out(FIFOtoMux); mux.in2(FIFOtoMux);
  mux.result(instruction);

  cache.address(address);

  cache.cacheHit(cacheHit);
  cache.isRoomToFetch(roomToFetch);
  cache.readInstruction(readFromCache);

  fifo.empty(fifoEmpty);
  fifo.readInstruction(readFromFIFO);

}

FetchStage::~FetchStage() {

}
