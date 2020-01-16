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
#include "SystemCall.h"

class ComputeTile;

namespace Compute {

class Core: public LokiComponent, public CoreInterface {

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

  // Implement CoreInterface.
  virtual void readRegister(RegisterIndex index, RegisterPort port);
  virtual void writeRegister(RegisterIndex index, RegisterFile::write_t value);

  virtual void readPredicate();
  virtual void writePredicate(PredicateRegister::write_t value);

  virtual void fetch(MemoryAddr address, ChannelMapEntry::MemoryChannel channel,
                     bool execute, bool persistent);
  virtual void jump(JumpOffset offset);

  virtual void computeLatency(opcode_t opcode, function_t fn=(function_t)0);

  virtual void readCMT(RegisterIndex index);
  virtual void writeCMT(RegisterIndex index, ChannelMapTable::write_t value);

  virtual void readCreg(RegisterIndex index);
  virtual void writeCreg(RegisterIndex index, ControlRegisters::write_t value);

  virtual void readScratchpad(RegisterIndex index);
  virtual void writeScratchpad(RegisterIndex index, Scratchpad::write_t value);

  virtual void syscall(int code);

  virtual void selectChannelWithData(uint bitmask);

  virtual void sendOnNetwork(NetworkData flit);
  virtual uint creditsAvailable(ChannelIndex channel) const;
  virtual void waitForCredit(ChannelIndex channel);

  virtual void startRemoteExecution(ChannelID address);
  virtual void endRemoteExecution();


  virtual ChannelID getNetworkDestination(EncodedCMTEntry channelMap,
                                          MemoryAddr address=0) const;

  virtual bool inputFIFOHasData(ChannelIndex fifo) const;


  const sc_event& readRegistersEvent(RegisterPort port) const;
  const sc_event& wroteRegistersEvent(RegisterPort port=REGISTER_PORT_1) const;
  const sc_event& computeFinishedEvent() const;
  const sc_event& readCMTEvent(RegisterPort port=REGISTER_PORT_1) const;
  const sc_event& wroteCMTEvent(RegisterPort port=REGISTER_PORT_1) const;
  const sc_event& readScratchpadEvent(RegisterPort port=REGISTER_PORT_1) const;
  const sc_event& wroteScratchpadEvent(RegisterPort port=REGISTER_PORT_1) const;
  const sc_event& readCRegsEvent(RegisterPort port=REGISTER_PORT_1) const;
  const sc_event& wroteCRegsEvent(RegisterPort port=REGISTER_PORT_1) const;
  const sc_event& readPredicateEvent(RegisterPort port=REGISTER_PORT_1) const;
  const sc_event& wrotePredicateEvent(RegisterPort port=REGISTER_PORT_1) const;
  const sc_event& networkDataArrivedEvent() const;
  const sc_event& sentNetworkDataEvent() const;
  const sc_event& creditArrivedEvent() const;
  const sc_event& selchFinishedEvent() const;

  const RegisterFile::read_t      getRegisterOutput(RegisterPort port) const;
  const ChannelMapTable::read_t   getCMTOutput(RegisterPort port=REGISTER_PORT_1) const;
  const Scratchpad::read_t        getScratchpadOutput(RegisterPort port=REGISTER_PORT_1) const;
  const ControlRegisters::read_t  getCRegOutput(RegisterPort port=REGISTER_PORT_1) const;
  const PredicateRegister::read_t getPredicateOutput(RegisterPort port=REGISTER_PORT_1) const;
  const RegisterIndex             getSelectedChannel() const;


  // Debug interface which bypasses instrumentation, etc. and completes
  // instantly.
  int32_t debugRegisterRead(RegisterIndex reg);
  int32_t debugReadByte(MemoryAddr address);
  void debugRegisterWrite(RegisterIndex reg, int32_t data);
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
