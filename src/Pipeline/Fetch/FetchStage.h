/*
 * FetchStage.h
 *
 * Fetch stage of the pipeline. Responsible for receiving instructions from
 * the network and feeding the decode stage a (near) constant supply of
 * instructions in the correct order.
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef FETCHSTAGE_H_
#define FETCHSTAGE_H_

#include "../PipelineStage.h"
#include "../../Multiplexor/Multiplexor2.h"
#include "InstructionPacketCache.h"
#include "InstructionPacketFIFO.h"

class FetchStage : public PipelineStage {

/* Components */
  InstructionPacketCache cache;
  InstructionPacketFIFO fifo;
  Multiplexor2<Instruction> mux;

/* Local state */
  bool usingCache;
  short selectVal;

/* Signals (wires) */
  sc_signal<Instruction> toCache, toFIFO, cacheToMux, FIFOtoMux, output;
  sc_buffer<short> muxSelect;
  sc_buffer<Address> addressSig;
  sc_signal<bool> cacheHitSig, fifoEmpty, readFromFIFO, readFromCache;

/* Methods */
  virtual void newCycle();
  void newFIFOInst();
  void newCacheInst();
  short calculateSelect();
  void select();

public:
/* Ports */
  sc_in<Word> toIPKCache, toIPKQueue;
  sc_in<Address> address;
  sc_out<Instruction> instruction;
  sc_out<bool> cacheHit, roomToFetch;

/* Constructors and destructors */
  SC_HAS_PROCESS(FetchStage);
  FetchStage(sc_core::sc_module_name name, int ID);
  virtual ~FetchStage();
};

#endif /* FETCHSTAGE_H_ */
