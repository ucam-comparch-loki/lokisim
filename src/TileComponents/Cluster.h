/*
 * Cluster.h
 *
 * A class representing an individual processing core. All work is done in the
 * subcomponents, and this class just serves to hold them all in one place and
 * connect them together correctly.
 *
 * There is currently an odd mix of SystemC signals and function calls within
 * a Cluster. This should be tidied up at some point to make things consistent.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef CLUSTER_H_
#define CLUSTER_H_

#include "TileComponent.h"
#include "../flag_signal.h"

#include "../Datatype/AddressedWord.h"
#include "../Datatype/DecodedInst.h"
#include "Pipeline/IndirectRegisterFile.h"
#include "Pipeline/PredicateRegister.h"
#include "Pipeline/Fetch/FetchStage.h"
#include "Pipeline/Decode/DecodeStage.h"
#include "Pipeline/Execute/ExecuteStage.h"
#include "Pipeline/Write/WriteStage.h"
#include "Pipeline/ChannelMapTable.h"

class InputCrossbar;
class StallRegister;

class Cluster : public TileComponent {

//==============================//
// Ports
//==============================//

public:

// Inherited from TileComponent:
//   clock
//   dataIn,      validDataIn,    readyOut;
//   dataOut,     validDataOut;
//   creditsOut,  validCreditOut, ackCreditOut;
//   creditsIn,   validCreditIn,  ackCreditIn;
//   idle

  // A slight hack to improve simulation speed. Each core contains a small
  // network at its input buffers, so we need to skew the times that the
  // network sends and receives data so data can get through the small
  // network and the larger tile network in one cycle.
  // In practice, these would probably be implemented as delays in the small
  // network.
  sc_in<bool> fastClock, slowClock;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Cluster);
  Cluster(sc_module_name name, const ComponentID& ID, local_net_t* network);
  virtual ~Cluster();

//==============================//
// Methods
//==============================//

public:

  // Initialise the instructions a Cluster will execute.
  virtual void     storeData(const std::vector<Word>& data, MemoryAddr location=0);

  // Returns the channel ID of the specified cluster's instruction packet FIFO.
  static ChannelID IPKFIFOInput(const ComponentID& ID);

  // Returns the channel ID of the specified cluster's instruction packet cache.
  static ChannelID IPKCacheInput(const ComponentID& ID);

  // Returns the channel ID of the specified cluster's input channel.
  static ChannelID RCETInput(const ComponentID& ID, ChannelIndex channel);

  // Get the memory location of the current instruction being decoded, so
  // we can have breakpoints set to particular instructions in memory.
  virtual const MemoryAddr  getInstIndex() const;

  // Read a value from a register, without redirecting to the RCET.
  virtual const int32_t  readRegDebug(RegisterIndex reg) const;

  // Read the value of the predicate register.
  // The optional wait parameter makes it possible to wait until the latest
  // predicate has been computed, if it will be written this cycle.
  virtual bool     readPredReg(bool wait=false);

  const Word readWord(MemoryAddr addr) const;
  const Word readByte(MemoryAddr addr) const;
  void writeWord(MemoryAddr addr, Word data);
  void writeByte(MemoryAddr addr, Word data);

private:

  // Determine if the instruction packet from the given location is currently
  // in the instruction packet cache.
  // There are many different ways of fetching instructions, so provide the
  // operation too.
  bool             inCache(const MemoryAddr addr, opcode_t operation);

  // Determine if there is room in the cache to fetch another instruction
  // packet, assuming that it is of maximum size. Also make sure there is not
  // another fetch already in progress.
  bool             readyToFetch() const;

  // Perform an IBJMP and jump to a new instruction in the cache.
  void             jump(const JumpOffset offset);

  // Read a value from a register. This method will redirect the request to
  // the receive channel-end table if the register index corresponds to a
  // channel end.
  const int32_t    readReg(RegisterIndex reg, bool indirect = false);

  // Read a value from a channel end. Warning: this removes the value from
  // the input buffer.
  const int32_t    readRCET(ChannelIndex channel);

  // Write a value to a register.
  void             writeReg(RegisterIndex reg, int32_t value,
                            bool indirect = false);

  // Write a value to the predicate register.
  void             writePredReg(bool val);

  // Update the address of the currently executing instruction packet so we
  // can fetch more packets from relative locations.
  void             updateCurrentPacket(MemoryAddr addr);

  // Tell the cluster whether it is currently stalled or not.
  void             pipelineStalled(bool stalled);

  // Discard the instruction waiting to enter the given pipeline stage (if any).
  // Returns whether or not anything was discarded.
  bool             discardInstruction(int stage);

  // Update whether this core is idle or not.
  void             updateIdle();

  // Request to reserve a path through the network to the given destination.
  // Returns an event which will be triggered when the request is granted.
  const sc_event&  requestArbitration(ChannelID destination, bool request);

  ComponentID      getSystemCallMemory() const;

//==============================//
// Components
//==============================//

private:

  // Very small crossbar between input ports and input buffers. Allows there to
  // be fewer network connections, making the tile network simpler.
  InputCrossbar*         inputCrossbar;

  IndirectRegisterFile   regs;
  PredicateRegister      pred;

  // Each of the pipeline stages.
  FetchStage             fetch;
  DecodeStage            decode;
  ExecuteStage           execute;
  WriteStage             write;

  // A stall register to go between each pair of adjacent pipeline stages.
  vector<StallRegister*> stallRegs;

  ChannelMapTable        channelMapTable;

  friend class IndirectRegisterFile;
  friend class FetchStage;
  friend class DecodeStage;
  friend class ExecuteStage;
  friend class WriteStage;

//==============================//
// Local state
//==============================//

private:

  bool currentlyStalled;
  sc_event stallEvent;

  // Store a pointer to the network so new ways of accessing it can be
  // experimented with without having to create lots of ports and signals.
  local_net_t* localNetwork;

//==============================//
// Signals (wires)
//==============================//

private:

  // A signal set to constantly hold "true".
  sc_signal<bool>            constantHigh;

  // Signals telling us which stages are idle, able to send data, or stalled.
  sc_signal<bool>           *stageIdle, *stallRegReady, *stageReady;

  sc_buffer<Word>           *dataToBuffers;
  sc_signal<bool>           *fcFromBuffers;

  // Transmission of the instruction along the pipeline. sc_buffers because we
  // want to trigger an event even if the instruction is identical.
  flag_signal<DecodedInst>  *instToStage;
  sc_buffer<DecodedInst>    *instFromStage;

};

#endif /* CLUSTER_H_ */
