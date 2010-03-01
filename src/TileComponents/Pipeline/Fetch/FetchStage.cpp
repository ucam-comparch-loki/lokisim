/*
 * FetchStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "FetchStage.h"

void FetchStage::storeCode(std::vector<Instruction>& instructions) {
  cache.storeCode(instructions);
}

void FetchStage::newCycle() {
  while(true) {
    wait(clock.posedge_event());

    if(!usingCache) readFromFIFO.write(!readFromFIFO.read());
    else readFromCache.write(!readFromCache.read());
  }
}

void FetchStage::newFIFOInst() {
  Instruction* instToFIFO = new Instruction(
                              static_cast<Instruction>(toIPKQueue.read()));

  if(DEBUG) cout<<"IPK FIFO received Instruction:  "<<*instToFIFO<<endl;

  toFIFO.write(*instToFIFO);
}

void FetchStage::newCacheInst() {
  Instruction* instToCache = new Instruction(
                              static_cast<Instruction>(toIPKCache.read()));

  if(DEBUG) cout<<"IPK Cache received Instruction: "<<*instToCache<<endl;

  toCache.write(*instToCache);
}

/* Choose whether to take an instruction from the cache or FIFO.
 * Use FIFO if already executing a packet from FIFO,
 *          or cache finished a packet and FIFO has instructions.
 * Use cache if already executing a packet from cache,
 *          or FIFO is empty and cache has instructions. */
short FetchStage::calculateSelect() {

  // 0 = cache, 1 = FIFO
  short result;

  if(usingCache) {
    Instruction i = cacheToMux.read();
    usingCache = fifoEmpty.read() || !i.endOfPacket();
    result = 0; //cache
  }
  else {
    usingCache = fifoEmpty.read();
    result = usingCache ? 0 : 1;
  }

  return result;

}

void FetchStage::select() {
  short sel = calculateSelect();
  muxSelect.write(sel);

  if(DEBUG) {
    printf("FetchStage selected instruction from %s: ", sel==0?"cache":"FIFO");
    if(sel) cout << FIFOtoMux.read() << endl;
    else cout << cacheToMux.read() << endl;
  }
}

FetchStage::FetchStage(sc_core::sc_module_name name) :
    PipelineStage(name),
    cache("IPKcache"),
    fifo("IPKfifo"),
    mux("fetchmux") {

  usingCache = true;
  flowControl = new sc_out<bool>[2];

// Register methods
  SC_METHOD(newFIFOInst);
  sensitive << toIPKQueue;
  dont_initialize();

  SC_METHOD(newCacheInst);
  sensitive << toIPKCache;
  dont_initialize();

  SC_METHOD(select);
  sensitive << cacheToMux << FIFOtoMux << clock.pos();
  dont_initialize();

// Connect everything up
  cache.clock(clock);
  fifo.clock(clock);

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
  cache.flowControl(flowControl[1]);

  fifo.empty(fifoEmpty);
  fifo.readInstruction(readFromFIFO);
  fifo.flowControl(flowControl[0]);

}

FetchStage::~FetchStage() {

}
