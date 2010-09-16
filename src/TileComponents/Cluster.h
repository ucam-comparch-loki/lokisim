/*
 * Cluster.h
 *
 * A class representing an individual processing core. All work is done in the
 * subcomponents, and this class just serves to hold them all in one place and
 * connect them together correctly.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef CLUSTER_H_
#define CLUSTER_H_

#include "TileComponent.h"
#include "../flag_signal.h"

#include "Pipeline/IndirectRegisterFile.h"
#include "Pipeline/PredicateRegister.h"
#include "Pipeline/Fetch/FetchStage.h"
#include "Pipeline/Decode/DecodeStage.h"
#include "Pipeline/Execute/ExecuteStage.h"
#include "Pipeline/Write/WriteStage.h"

class Cluster : public TileComponent {

//==============================//
// Ports
//==============================//

// Inherited from TileComponent:
//   clock
//   in
//   out
//   flowControlIn
//   flowControlOut
//   idle

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Cluster);
  Cluster(sc_module_name name, uint16_t ID);
  virtual ~Cluster();

//==============================//
// Methods
//==============================//

public:

  virtual double   area() const;
  virtual double   energy() const;

  // Initialise the instructions a Cluster will execute.
  virtual void     storeData(std::vector<Word>& data);

  // Returns the channel ID of the specified cluster's instruction packet FIFO.
  static uint32_t  IPKFIFOInput(uint16_t ID);

  // Returns the channel ID of the specified cluster's instruction packet cache.
  static uint32_t  IPKCacheInput(uint16_t ID);

  // Returns the channel ID of the specified cluster's input channel.
  static uint32_t  RCETInput(uint16_t ID, ChannelIndex channel);

  // Get the memory location of the current instruction being decoded, so
  // we can have breakpoints set to particular instructions in memory.
  virtual uint16_t getInstIndex() const;

  // Read a value from a register.
  virtual int32_t  readReg(RegisterIndex reg, bool indirect = false) const;

  // Read the value of the predicate register.
  virtual bool     readPredReg() const;

private:

  // Determine if the instruction packet from the given location is currently
  // in the instruction packet cache.
  bool             inCache(Address a);

  // Determine if there is room in the cache to fetch another instruction
  // packet, assuming that it is of maximum size.
  bool             roomToFetch() const;

  // An instruction packet was in the cache, and set to execute next, but has
  // since been overwritten, so needs to be fetched again.
  void             refetch();

  // Perform an IBJMP and jump to a new instruction in the cache.
  void             jump(int8_t offset);

  // Set whether the cache is in persistent or non-persistent mode.
  void             setPersistent(bool persistent);

  // Write a value to a register.
  void             writeReg(RegisterIndex reg, int32_t value,
                            bool indirect = false);

  // Write a value to the predicate register.
  void             writePredReg(bool val);

  // Read a value from a channel end. Warning: this removes the value from
  // the input buffer.
  int32_t          readRCET(ChannelIndex channel);

  // Update the address of the currently executing instruction packet so we
  // can fetch more packets from relative locations.
  void             updateCurrentPacket(Address addr);

  // One of the pipeline stages has changed its stall status -- see if the
  // whole pipeline should now be stalled or unstalled.
  void             stallPipeline();

  // Update whether this core is idle or not.
  void             updateIdle();

//==============================//
// Components
//==============================//

private:

  IndirectRegisterFile     regs;
  PredicateRegister        pred;
  FetchStage               fetch;
  DecodeStage              decode;
  ExecuteStage             execute;
  WriteStage               write;

  friend class IndirectRegisterFile;
  friend class FetchStage;
  friend class DecodeStage;
  friend class ExecuteStage;
  friend class WriteStage;

//==============================//
// Signals (wires)
//==============================//

private:

  sc_signal<bool>          stallSig;
  sc_signal<bool>          fetchIdle, decodeIdle, executeIdle, writeIdle;

  // "Flow control" within the pipeline.
  sc_signal<bool>          decodeReady, executeReady, writeReady;

  // Transmission of the instruction along the pipeline.
  flag_signal<Instruction> fetchToDecode;
  flag_signal<DecodedInst> decodeToExecute, executeToWrite;


  // Not needed anymore? We now stall individual stages. Or maybe just use
  // fetch's signal to determine if whole cluster is stalled?
  sc_signal<bool>          fetchStallSig, decStallSig,
                           stallFetchSig, writeStallSig;

};

#endif /* CLUSTER_H_ */
