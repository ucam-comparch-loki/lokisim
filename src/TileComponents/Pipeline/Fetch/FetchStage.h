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
#include "../../../Datatype/Instruction.h"
#include "../../../Network/NetworkTypedefs.h"

class FetchStage : public PipelineStage {

//==============================//
// Ports
//==============================//

public:

// Inherited from PipelineStage:
//   sc_in<bool>         clock
//   sc_out<bool>        idle

  // The input instruction to be sent to the instruction packet cache.
  sc_in<Word>         toIPKCache;

  // The input instruction to be sent to the instruction packet FIFO.
  sc_in<Word>         toIPKFIFO;

  // A flow control signal from each of the two instruction inputs.
  LokiVector<ReadyOutput> flowControl;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FetchStage);
  FetchStage(sc_module_name name, const ComponentID& ID);

//==============================//
// Methods
//==============================//

public:

  // Store some instructions in the Instruction Packet Cache.
  void          storeCode(const std::vector<Instruction>& instructions);

  // Return the memory address of the last instruction sent.
  MemoryAddr    getInstIndex() const;

  // Tells whether the packet from location a is currently in the cache.
  // There are many different ways of fetching instructions, so provide the
  // operation too.
  bool          inCache(const MemoryAddr a, opcode_t operation);

  // Tells whether there is room in the cache to fetch another instruction
  // packet, assuming the packet is of maximum size.
  bool          roomToFetch() const;

  // Jump to a different instruction in the Instruction Packet Cache.
  void          jump(const JumpOffset offset);

private:

  virtual void  execute();

  // The fetch stage needs to be sure that other tasks have completed before
  // reading from the cache, so waits until later in the cycle to do it.
  void          getInstruction();

  // Recompute whether this pipeline stage is stalled.
  virtual void  updateReady();

  // Determine whether to take an instruction from the FIFO or cache.
  void          calculateSelect();

  // Move to the next instruction packet on demand, rather than when reaching
  // a ".eop" marker.
  void          nextIPK();

  // Override PipelineStage's implementation.
  virtual void  getNextInstruction();

//==============================//
// Components
//==============================//

private:

  InstructionPacketCache    cache;
  InstructionPacketFIFO     fifo;

  friend class InstructionPacketCache;
  friend class InstructionPacketFIFO;

//==============================//
// Local state
//==============================//

private:

  // If this pipeline stage is stalled, we assume the whole pipeline is stalled.
  bool stalled;

  // Tells whether the last operation read from the cache or not.
  bool usingCache;

};

#endif /* FETCHSTAGE_H_ */
