/*
 * Core.h
 *
 *  Created on: Aug 16, 2019
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_CORE_H_
#define SRC_TILE_CORE_CORE_H_

#include "../../ISA/CoreInterface.h"
#include "../../LokiComponent.h"
#include "../../Utility/LokiVector.h"
#include "../Core/Fetch/InstructionPacketCache.h"
#include "../Core/Fetch/InstructionPacketFIFO.h"
#include "../Core/MagicMemoryConnection.h"
#include "../MemoryBankSelector.h"
#include "FetchStage.h"
#include "DecodeStage.h"
#include "ExecuteStage.h"
#include "WriteStage.h"
#include "Storage.h"
#include "FIFOArray.h"
#include "ForwardingNetwork.h"
#include "PipelineRegister.h"
#include "SystemCall.h"

class ComputeTile;

namespace Compute {

// TODO: don't implement CoreInterface, and instead pass DecodedInstructions
// around to the various parts of the core.
class Core: public LokiComponent {

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

  Core(const sc_module_name& name, const ComponentID& ID,
       const core_parameters_t& params, size_t numMulticastInputs,
       size_t numMulticastOutputs, size_t numMemories);

//============================================================================//
// Methods
//============================================================================//

public:

  void readRegister(DecodedInstruction inst, RegisterIndex index,
                    PortIndex port);
  void writeRegister(DecodedInstruction inst, RegisterIndex index,
                     RegisterFile::write_t value);

  void readPredicate(DecodedInstruction inst);
  void writePredicate(DecodedInstruction inst,
                      PredicateRegister::write_t value);

  void fetch(DecodedInstruction inst, MemoryAddr address,
             ChannelMapEntry::MemoryChannel channel, bool execute,
             bool persistent);
  void jump(DecodedInstruction inst, JumpOffset offset);

  void readCMT(DecodedInstruction inst, RegisterIndex index, PortIndex port);
  void writeCMT(DecodedInstruction inst, RegisterIndex index,
                ChannelMapTable::write_t value);

  void readCreg(DecodedInstruction inst, RegisterIndex index);
  void writeCreg(DecodedInstruction inst, RegisterIndex index,
                 ControlRegisters::write_t value);

  void readScratchpad(DecodedInstruction inst, RegisterIndex index);
  void writeScratchpad(DecodedInstruction inst, RegisterIndex index,
                       Scratchpad::write_t value);

  void syscall(DecodedInstruction inst, int code);

  void selectChannelWithData(DecodedInstruction inst, uint bitmask);

  void sendOnNetwork(DecodedInstruction inst, NetworkData flit);
  void waitForCredit(DecodedInstruction inst, ChannelIndex channel);


  uint creditsAvailable(ChannelIndex channel) const;
  ChannelID getNetworkDestination(EncodedCMTEntry channelMap,
                                  MemoryAddr address=0) const;
  bool inputFIFOHasData(ChannelIndex fifo) const;


  // Debug interface which bypasses instrumentation, etc. and completes
  // instantly.
  RegisterFile::read_t debugRegisterRead(RegisterIndex reg);
  void debugRegisterWrite(RegisterIndex reg, RegisterFile::write_t data);
  int32_t debugReadByte(MemoryAddr address);
  void debugWriteByte(MemoryAddr address, int32_t data);

  uint coreIndex() const;
  uint coresThisTile() const;
  uint globalCoreIndex() const;

  ComputeTile& tile() const;

  ComponentID getSystemCallMemory(MemoryAddr address) const;

//============================================================================//
// Components
//============================================================================//

public:

  // Unique identifier for this core.
  const ComponentID id;

private:

  // Pipeline stages.
  FetchStage             fetchStage;
  DecodeStage            decodeStage;
  ExecuteStage           executeStage;
  WriteStage             writeStage;

  PipelineRegister       pipeReg1, pipeReg2, pipeReg3;
  ForwardingNetwork      forwarding;

  // Storage structures.
  RegisterFile           registers;
  Scratchpad             scratchpad;
  ChannelMapTable        channelMapTable;
  ControlRegisters       controlRegisters;
  PredicateRegister      predicate;

  // Network connections.
  InstructionPacketFIFO  iInstFIFO;
  InstructionPacketCache iInstCache;
  InputFIFOs             iDataFIFOs;

  OutputFIFOs            oDataFIFOs;

  MemoryBankSelector     memoryBankSelector;

  // Debug connection to memory. Has zero latency.
  MagicMemoryConnection  magicMemoryConnection;

  SystemCall             systemCallHandler;

};

} // end namespace

#endif /* SRC_TILE_CORE_CORE_H_ */
