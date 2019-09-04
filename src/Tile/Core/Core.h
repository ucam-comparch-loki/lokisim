/*
 * Core.h
 *
 * A class representing an individual processing core. All work is done in the
 * subcomponents, and this class just serves to hold them all in one place and
 * connect them together correctly.
 *
 * There is currently an odd mix of SystemC signals and function calls within
 * a Core. This should be tidied up at some point to make things consistent.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef CORE_H_
#define CORE_H_

#include "../../Network/FIFOs/NetworkFIFO.h"
#include "../../Network/NetworkTypes.h"
#include "../../Utility/LokiVector.h"
#include "ChannelMapTable.h"
#include "ControlRegisters.h"
#include "Decode/DecodeStage.h"
#include "Execute/ExecuteStage.h"
#include "Fetch/FetchStage.h"
#include "PredicateRegister.h"
#include "RegisterFile.h"
#include "Write/WriteStage.h"
#include "MagicMemoryConnection.h"
#include "PipelineRegister.h"

class ComputeTile;
class DecodedInst;

class Core : public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  // TODO: It would be nice if the instruction inputs were a different type, but
  // I don't know if the type system will allow Word ports to connect to
  // Instruction ones.
  // Allow each port to be bound to multiple networks. Having multiple writers
  // to a single buffer must be managed in software, so no arbitration or
  // checks are provided.
  typedef sc_port<network_sink_ifc<Word>, 0> InPort;
  typedef sc_port<network_source_ifc<Word>> OutPort;

  // Clock.
  ClockInput            clock;

  // Data inputs/outputs from network(s).
  LokiVector<InPort>    iData;
  OutPort               oMemory;
  OutPort               oMulticast;

  // Connections to the global credit network.
  // Credits are sent on the Core's behalf by the Intertile Communication Unit,
  // so only an input port is needed here.
  InPort                iCredit;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(Core);
  Core(const sc_module_name& name, const ComponentID& ID,
       const core_parameters_t& params, size_t numMulticastInputs,
       size_t numMulticastOutputs, size_t numMemories);

//============================================================================//
// Methods
//============================================================================//

public:

  // Initialise the instructions a Core will execute.
  virtual void     storeData(const std::vector<Word>& data);

  // Returns the channel ID of the specified core's instruction packet FIFO.
  static ChannelID IPKFIFOInput(const ComponentID& ID);

  // Returns the channel ID of the specified core's instruction packet cache.
  static ChannelID IPKCacheInput(const ComponentID& ID);

  // Returns the channel ID of the specified core's input channel.
  static ChannelID RCETInput(const ComponentID& ID, ChannelIndex channel);

  // Get the memory location of the current instruction being decoded, so
  // we can have breakpoints set to particular instructions in memory.
  virtual MemoryAddr  getInstIndex() const;

  // Read a value from a register, without redirecting to the RCET.
  virtual int32_t  readRegDebug(RegisterIndex reg) const;

  // Read the value of the predicate register.
  // The optional wait parameter makes it possible to wait until the latest
  // predicate has been computed, if it will be written this cycle.
  virtual bool     readPredReg(bool wait=false, const DecodedInst& inst = DecodedInst());

  const Word readWord(MemoryAddr addr);
  const Word readByte(MemoryAddr addr);
  void writeWord(MemoryAddr addr, Word data);
  void writeByte(MemoryAddr addr, Word data);
  void magicMemoryAccess(const DecodedInst& instruction);
  void magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address, ChannelID returnChannel, Word payload = 0);

  // Receive data over the magic, zero-latency network.
  void deliverDataInternal(const NetworkData& flit);
  void deliverCreditInternal(const NetworkCredit& flit);

  // The number of input buffers, excluding any reserved for instructions.
  size_t numInputDataBuffers() const;

  // The index of this core, with the first core being 0.
  uint coreIndex() const;
  uint coresThisTile() const;
  uint globalCoreIndex() const;

  // Get information about other components. Mostly for debug/stats.
  bool isCore(ComponentID id) const;
  bool isMemory(ComponentID id) const;
  bool isComputeTile(TileID id) const;

  ComputeTile& parent() const;

private:

  // Return whether it is possible to check the cache tags at this time. It may
  // not be possible if there is already the maximum number of packets queued up.
  bool             canCheckTags() const;

  // Queue up an instruction packet to be fetched. Include all necessary
  // information to send a fetch request to memory, if needed.
  void             checkTags(MemoryAddr addr, opcode_t op, EncodedCMTEntry netInfo);

  // Perform an IBJMP and jump to a new instruction in the cache.
  void             jump(const JumpOffset offset);

  // Read a value from a register. This method will redirect the request to
  // the receive channel-end table if the register index corresponds to a
  // channel end.
  int32_t          readReg(PortIndex port, RegisterIndex reg, bool indirect = false);

  // Read a value from a channel end. Warning: this removes the value from
  // the input buffer.
  int32_t          readRCET(ChannelIndex channel);

  // Return the result of the instruction in the execute stage.
  int32_t          getForwardedData() const;

  // Write a value to a register.
  void             writeReg(RegisterIndex reg, int32_t value,
                            bool indirect = false);

  // Write a value to the predicate register.
  void             writePredReg(bool val);

  // Update the address of the currently executing instruction packet so we
  // can fetch more packets from relative locations.
  void             updateCurrentPacket(MemoryAddr addr);

  // Update the contents of the control register responsible for recording the
  // latest "normal" fetch.
  // https://svr-rdm34-issue.cl.cam.ac.uk/w/loki/architecture/core/decode/
  void             updateFetchAddressCReg(MemoryAddr addr);

  // Tell the core whether it is currently stalled or not.
  void             pipelineStalled(bool stalled);

  // Discard the instruction waiting to enter the given pipeline stage (if any).
  // Returns whether or not anything was discarded.
  bool             discardInstruction(int stage);

  // Finish executing the current instruction packet immediately, rather than
  // waiting for an ".eop" marker.
  void             nextIPK();

  // Method triggered whenever credits arrive on iCredit.
  void             receivedCredit();

  // Update whether this core is idle or not.
  void             idle(bool state);

  // Print out information about the environment this instruction executed in.
  void             trace(const DecodedInst& instruction) const;

  // The memory bank which is responsible for holding the given address.
  // Assumes a data cache connection is on channel 1.
  ComponentID      getSystemCallMemory(MemoryAddr address);

//============================================================================//
// Components
//============================================================================//

public:
  // Pipeline stages.
  FetchStage             fetch;
  DecodeStage            decode;
  ExecuteStage           execute;
  WriteStage             write;

private:

  RegisterFile           regs;
  PredicateRegister      pred;

  // A pipeline register to go between each pair of adjacent stages.
  LokiVector<PipelineRegister> pipelineRegs;

  ChannelMapTable        channelMapTable;
  ControlRegisters       cregs;

  // Credits received from the network.
  NetworkFIFO<Word>      incomingCredits;

  // Debug connection to memory. Has zero latency.
  MagicMemoryConnection  magicMemoryConnection;

  friend class RegisterFile;
  friend class FetchStage;
  friend class DecodeStage;
  friend class Decoder;
  friend class ReceiveChannelEndTable;
  friend class ExecuteStage;
  friend class WriteStage;
  friend class ControlRegisters;
  friend class MagicMemoryConnection;

//============================================================================//
// Local state
//============================================================================//

public:

  // Number of input channels reserved for instructions.
  static const uint numInstructionChannels = 2;

  const ComponentID id;

private:

  bool currentlyStalled;
  sc_event stallEvent;

//============================================================================//
// Signals (wires)
//============================================================================//

private:

  // Signals telling us which stages are able to send data or stalled.
  LokiVector<ReadySignal>      stageReady;

  // Data being sent to the output buffer.
  DataSignal                   fetchFlitSignal, dataFlitSignal;

};

#endif /* CORE_H_ */
