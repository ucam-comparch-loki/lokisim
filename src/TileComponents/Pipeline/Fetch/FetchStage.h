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
#include "InstructionPacketCache.h"
#include "InstructionPacketFIFO.h"

class FetchStage : public PipelineStage {

//==============================//
// Ports
//==============================//

public:

// Inherited from PipelineStage:
//   clock
//   stall
//   idle

  sc_in<bool>         readyIn;

  // The input instruction to be sent to the instruction packet cache.
  sc_in<Word>         toIPKCache;

  // The input instruction to be sent to the instruction packet FIFO.
  sc_in<Word>         toIPKFIFO;

  // The instruction selected for the rest of the pipeline to execute.
  sc_out<Instruction> instruction;

  // A flow control signal from each of the two instruction inputs.
  sc_out<int>        *flowControl;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FetchStage);
  FetchStage(sc_module_name name);
  virtual ~FetchStage();

//==============================//
// Methods
//==============================//

public:

  virtual double area()  const;
  virtual double energy() const;

  // Store some instructions in the Instruction Packet Cache.
  void          storeCode(std::vector<Instruction>& instructions);

  // Return the memory address of the last instruction sent.
  int           getInstIndex() const;

  bool          inCache(Address a);

  bool          roomToFetch() const;

  // Jump to a different instruction in the Instruction Packet Cache.
  void          jump(int8_t offset);

  void          setPersistent(bool persistent);

private:

  // The task performed at the beginning of each clock cycle.
  virtual void  newCycle();

  // Pass the newly-received instruction to the FIFO.
  void          newFIFOInst();

  // Pass the newly-received instruction to the cache.
  void          newCacheInst();

  // Determine whether to take an instruction from the FIFO or cache.
  short         calculateSelect();

  // Write the result of calculateSelect() to the multiplexors select input.
  void          select();

//==============================//
// Components
//==============================//

private:

  InstructionPacketCache    cache;
  InstructionPacketFIFO     fifo;

//==============================//
// Local state
//==============================//

private:

  // Tells whether the last operation read from the cache or not.
  bool usingCache;

  bool justFinishedPacket;

  Instruction lastInstruction;

  enum InstructionSource {CACHE, FIFO};

//==============================//
// Signals (wires)
//==============================//

private:

  sc_buffer<Instruction>    toCache, toFIFO;
  flag_signal<Instruction>  cacheToMux, FIFOtoMux;
  sc_buffer<short>          muxSelect;
  sc_signal<bool>           fifoEmpty, readFromFIFO, readFromCache;
  sc_buffer<short>          jumpOffsetSig;

};

#endif /* FETCHSTAGE_H_ */
