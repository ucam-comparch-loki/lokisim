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

#include "../Datatype/AddressedWord.h"
#include "Pipeline/IndirectRegisterFile.h"
#include "Pipeline/PredicateRegister.h"
#include "Pipeline/Fetch/FetchStage.h"
#include "Pipeline/Decode/DecodeStage.h"
#include "Pipeline/Execute/ExecuteStage.h"
#include "Pipeline/Write/WriteStage.h"

class InputCrossbar;
class StallRegister;

class Cluster : public TileComponent {

//==============================//
// Ports
//==============================//

// Inherited from TileComponent:
//   clock
//   dataIn       canReceiveData
//   dataOut      canSendData
//   creditsIn    canReceiveCredit
//   creditsOut   canSendCredit
//   idle

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Cluster);
  Cluster(sc_module_name name, const ComponentID& ID);
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
  virtual bool     readPredReg() const;

  const Word readWord(MemoryAddr addr) const;
  const Word readByte(MemoryAddr addr) const;
  void writeWord(MemoryAddr addr, Word data);
  void writeByte(MemoryAddr addr, Word data);

private:

  // Determine if the instruction packet from the given location is currently
  // in the instruction packet cache.
  bool             inCache(const MemoryAddr a);

  // Determine if there is room in the cache to fetch another instruction
  // packet, assuming that it is of maximum size.
  bool             roomToFetch() const;

  // An instruction packet was in the cache, and set to execute next, but has
  // since been overwritten, so needs to be fetched again.
  void             refetch();

  // Perform an IBJMP and jump to a new instruction in the cache.
  void             jump(const JumpOffset offset);

  // Set whether the cache is in persistent or non-persistent mode.
  void             setPersistent(bool persistent);

  // Read a value from a register. This method will redirect the request to
  // the receive channel-end table if the register index corresponds to a
  // channel end.
  const int32_t    readReg(RegisterIndex reg, bool indirect = false) const;

  // Read a value from a channel end. Warning: this removes the value from
  // the input buffer.
  const int32_t    readRCET(ChannelIndex channel);

  // Write a value to a register.
  void             writeReg(RegisterIndex reg, int32_t value,
                            bool indirect = false);

  // Write a value to the predicate register.
  void             writePredReg(bool val);

  // Check the forwarding paths to see if this instruction requires values
  // which haven't been written to registers yet.
  void             checkForwarding(DecodedInst& inst) const;

  // Update the values in the forwarding paths using a new result.
  void             updateForwarding(const DecodedInst& inst);

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

//==============================//
// Components
//==============================//

private:

  // Very small crossbar between input ports and input buffers. Allows there to
  // be fewer network connections, making the tile network simpler.
  InputCrossbar*           inputCrossbar;

  IndirectRegisterFile     regs;
  PredicateRegister        pred;

  // Each of the pipeline stages.
  FetchStage   fetch;
  DecodeStage  decode;
  ExecuteStage execute;
  WriteStage   write;

  // A stall register to go between each pair of adjacent pipeline stages.
  vector<StallRegister*>   stallRegs;

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

  // Store the previous two results and destination registers, to allow data
  // forwarding when necessary. 2 is older than 1.
  RegisterIndex previousDest1, previousDest2;
  int64_t previousResult1, previousResult2;

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
  flag_signal<DecodedInst>  *dataToStage;
  sc_buffer<DecodedInst>    *dataFromStage;

  // Signal going straight from fetch logic to output buffers.
  flag_signal<AddressedWord> fetchSignal;

};

#endif /* CLUSTER_H_ */
