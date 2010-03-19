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
#include "../../../Multiplexor/Multiplexor2.h"
#include "InstructionPacketCache.h"
#include "InstructionPacketFIFO.h"

class FetchStage : public PipelineStage {

//==============================//
// Ports
//==============================//

public:

  // The input instruction to be sent to the instruction packet cache.
  sc_in<Word>         toIPKCache;

  // The input instruction to be sent to the instruction packet FIFO.
  sc_in<Word>         toIPKQueue;

  // The instruction selected for the rest of the pipeline to execute.
  sc_out<Instruction> instruction;

  // The address tag to lookup in the instruction packet cache.
  sc_in<Address>      address;

  // Status signals from the instruction packet cache.
  sc_out<bool>        cacheHit, roomToFetch;

  // A flow control signal from each of the two instruction inputs.
  sc_out<bool>       *flowControl;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FetchStage);
  FetchStage(sc_core::sc_module_name name);
  virtual ~FetchStage();

//==============================//
// Methods
//==============================//

public:

  void          storeCode(std::vector<Instruction>& instructions);

private:

  virtual void  newCycle();
  void          newFIFOInst();
  void          newCacheInst();
  short         calculateSelect();
  void          select();

//==============================//
// Components
//==============================//

private:

  InstructionPacketCache    cache;
  InstructionPacketFIFO     fifo;
  Multiplexor2<Instruction> mux;

//==============================//
// Local state
//==============================//

private:

  bool usingCache;

//==============================//
// Signals (wires)
//==============================//

private:

  sc_signal<Instruction>    toCache, toFIFO, cacheToMux, FIFOtoMux;
  sc_buffer<short>          muxSelect;
  sc_signal<bool>           fifoEmpty, readFromFIFO, readFromCache;

};

#endif /* FETCHSTAGE_H_ */
