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

#include "../StageWithSuccessor.h"
#include "InstructionPacketCache.h"
#include "InstructionPacketFIFO.h"
#include "../../../Datatype/Instruction.h"

class DecodedInst;

class FetchStage : public StageWithSuccessor {

//==============================//
// Ports
//==============================//

public:

// Inherited from StageWithSuccessor:
//   sc_in<bool>         clock
//   sc_out<bool>        idle
//   sc_out<DecodedInst> dataOut;

  // The input instruction to be sent to the instruction packet cache.
  sc_in<Word>         toIPKCache;

  // The input instruction to be sent to the instruction packet FIFO.
  sc_in<Word>         toIPKFIFO;

  // A flow control signal from each of the two instruction inputs.
  sc_out<int>        *flowControl;

  // Tells whether the next stage is ready to receive a new instruction.
  sc_in<bool>         readyIn;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FetchStage);
  FetchStage(sc_module_name name, ComponentID ID);
  virtual ~FetchStage();

//==============================//
// Methods
//==============================//

public:

  virtual double area()   const;
  virtual double energy() const;

  // Store some instructions in the Instruction Packet Cache.
  void          storeCode(std::vector<Instruction>& instructions);

  // Return the memory address of the last instruction sent.
  Address       getInstIndex() const;

  // Tells whether the packet from location a is currently in the cache.
  bool          inCache(Address a);

  // Tells whether there is room in the cache to fetch another instruction
  // packet, assuming the packet is of maximum size.
  bool          roomToFetch() const;

  // Jump to a different instruction in the Instruction Packet Cache.
  void          jump(JumpOffset offset);

  // Put the cache into persistent mode, where it executes the same instruction
  // packet repeatedly, or take it out of persistent mode.
  void          setPersistent(bool persistent);

private:

  // If any new instructions have arrived, pass them to the corresponding components.
  virtual void  newCycle();

  // The fetch stage needs to be sure that other tasks have completed before
  // reading from the cache, so waits until later in the cycle to do it.
  virtual void  cycleSecondHalf();

  // Send out initial flow control values.
  virtual void  initialise();

  // Recompute whether this pipeline stage is stalled.
  virtual void  updateStall();

  // Determine whether to take an instruction from the FIFO or cache.
  void          calculateSelect();

  // Update the register holding the address of the current packet.
  void          updatePacketAddress(Address addr);

  // We have overwritten the packet due to execute next, so it needs to be
  // fetched again.
  void          refetch();

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

  // Tells whether the last instruction sent was the end of a packet.
  bool justFinishedPacket;

  // The most-recently-sent instruction.
  Instruction lastInstruction;

};

#endif /* FETCHSTAGE_H_ */
