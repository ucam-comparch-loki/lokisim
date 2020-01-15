/*
 * Core.cpp
 *
 *  Created on: Aug 16, 2019
 *      Author: db434
 */

#include "Core.h"
#include "../ComputeTile.h"
#include "../../Utility/Assert.h"

namespace Compute {

Core::Core(const sc_module_name& name, const ComponentID& ID,
           const core_parameters_t& params, size_t numMulticastInputs,
           size_t numMulticastOutputs, size_t numMemories) :
    LokiComponent(name),
    clock("clock"),
    iData("iData", params.numInputChannels),
    oMemory("oMemory"),
    oMulticast("oMulticast"),
    iCredit("iCredit"),
    id(ID),
    fetchStage("fetch"),
    decodeStage("decode"),
    executeStage("execute"),
    writeStage("write"),
    registers("regs", params.registerFile),
    scratchpad("scratchpad", params.scratchpad),
    channelMapTable("cmt", params.channelMapTable),
    controlRegisters("cregs"),
    predicate("predicate"),
    iInstFIFO("ipk_fifo", params.ipkFIFO),
    iInstCache("ipk_cache", params.cache),
    iDataFIFOs("iBuffers", params.numInputChannels - 2, params.inputFIFO),
    oDataFIFOs("oBuffers", params.outputFIFO),
    magicMemoryConnection("magic_memory"),
    systemCallHandler("syscall") {

  registers.clock(clock);
  scratchpad.clock(clock);
  channelMapTable.clock(clock);
  controlRegisters.clock(clock);
  predicate.clock(clock);

  iData[0](iInstFIFO);
  iData[1](iInstCache);
  for (uint i=2; i<params.numInputChannels; i++)
    iData[i](iDataFIFOs.iData[i-2]);
  iCredit(channelMapTable.iCredit);
  oMemory(oDataFIFOs.oMemory);
  oMulticast(oDataFIFOs.oMulticast);
}

void Core::readRegister(RegisterIndex index, RegisterPort port) {
  registers.read(index, port);
}
void Core::writeRegister(RegisterIndex index, RegisterFile::write_t value) {
  registers.write(index, value);
}

void Core::readPredicate() {
  predicate.read();
}
void Core::writePredicate(PredicateRegister::write_t value) {
  predicate.write(value);
}

void Core::fetch(MemoryAddr address, ChannelMapEntry::MemoryChannel channel,
                 bool execute, bool persistent) {
  // TODO
}
void Core::jump(JumpOffset offset) {
  // TODO
}

void Core::computeLatency(opcode_t opcode, function_t fn) {
  LOKI_ERROR << "Can't compute in " << this->name() << endl;
  loki_assert(false);
}

void Core::readCMT(RegisterIndex index) {
  channelMapTable.read(index);
}
void Core::writeCMT(RegisterIndex index, ChannelMapTable::write_t value) {
  channelMapTable.write(index, value);
}

void Core::readCreg(RegisterIndex index) {
  controlRegisters.read(index);
}
void Core::writeCreg(RegisterIndex index, ControlRegisters::write_t value) {
  controlRegisters.write(index, value);
}

void Core::readScratchpad(RegisterIndex index) {
  scratchpad.read(index);
}
void Core::writeScratchpad(RegisterIndex index, Scratchpad::write_t value) {
  scratchpad.write(index, value);
}

void Core::syscall(int code) {
  systemCallHandler.call((SystemCall::Code)code);
}

void Core::selectChannelWithData(uint bitmask) {
  iDataFIFOs.selectChannelWithData(bitmask);
}

void Core::sendOnNetwork(NetworkData flit) {
  oDataFIFOs.write(flit);
}
uint Core::creditsAvailable(ChannelIndex channel) const {
  return channelMapTable.creditsAvailable(channel);
}
void Core::waitForCredit(ChannelIndex channel) {
  LOKI_ERROR << "Can't wait for credits from " << this->name() << endl;
  loki_assert(false);
}

void Core::startRemoteExecution(ChannelID address) {
  LOKI_ERROR << "Can't start remote execution from " << this->name() << endl;
  loki_assert(false);
}
void Core::endRemoteExecution() {
  LOKI_ERROR << "Can't end remote execution from " << this->name() << endl;
  loki_assert(false);
}


ChannelID Core::getNetworkDestination(EncodedCMTEntry channelMap,
                                      MemoryAddr address) const {
  // TODO - I think MemoryBankSelector is in another branch?
}

bool Core::inputFIFOHasData(ChannelIndex fifo) const {
  return iDataFIFOs.hasData(fifo);
}


const sc_event& Core::readRegistersEvent(RegisterPort port) const {
  return registers.readPort[port].finished();
}
const sc_event& Core::wroteRegistersEvent(RegisterPort port) const {
  return registers.writePort[port].finished();
}
const sc_event& Core::computeFinishedEvent() const {
  // TODO
}
const sc_event& Core::readCMTEvent(RegisterPort port) const {
  return channelMapTable.readPort[port].finished();
}
const sc_event& Core::wroteCMTEvent(RegisterPort port) const {
  return channelMapTable.writePort[port].finished();
}
const sc_event& Core::readScratchpadEvent(RegisterPort port) const {
  return scratchpad.readPort[port].finished();
}
const sc_event& Core::wroteScratchpadEvent(RegisterPort port) const {
  return scratchpad.writePort[port].finished();
}
const sc_event& Core::readCRegsEvent(RegisterPort port) const {
  return controlRegisters.readPort[port].finished();
}
const sc_event& Core::wroteCRegsEvent(RegisterPort port) const {
  return controlRegisters.writePort[port].finished();
}
const sc_event& Core::readPredicateEvent(RegisterPort port) const {
  return predicate.readPort[port].finished();
}
const sc_event& Core::wrotePredicateEvent(RegisterPort port) const {
  return predicate.writePort[port].finished();
}
const sc_event& Core::networkDataArrivedEvent() const {
  return iDataFIFOs.anyDataArrivedEvent();
}
const sc_event& Core::sentNetworkDataEvent() const {
  return oDataFIFOs.anyDataSentEvent();
}
const sc_event& Core::creditArrivedEvent() const {
  return channelMapTable.creditArrivedEvent();
}
const sc_event& Core::selchFinishedEvent() const {
  return iDataFIFOs.selectedDataArrivedEvent();
}

const RegisterFile::read_t      Core::getRegisterOutput(RegisterPort port) const {
  return registers.readPort[port].result();
}
const ChannelMapTable::read_t   Core::getCMTOutput(RegisterPort port) const {
  return channelMapTable.readPort[port].result();
}
const Scratchpad::read_t        Core::getScratchpadOutput(RegisterPort port) const {
  return scratchpad.readPort[port].result();
}
const ControlRegisters::read_t  Core::getCRegOutput(RegisterPort port) const {
  return controlRegisters.readPort[port].result();
}
const PredicateRegister::read_t Core::getPredicateOutput(RegisterPort port) const {
  return predicate.readPort[port].result();
}
const RegisterIndex             Core::getSelectedChannel() const {
  return iDataFIFOs.getSelectedChannel();
}



int32_t Core::debugRegisterRead(RegisterIndex reg) {
  return registers.debugRead(reg);
}
int32_t Core::debugReadByte(MemoryAddr address) {
  return tile().readByteInternal(getSystemCallMemory(address), address).toInt();
}
void Core::debugRegisterWrite(RegisterIndex reg, int32_t data) {
  registers.debugWrite(reg, data);
}
void Core::debugWriteByte(MemoryAddr address, int32_t data) {
  tile().writeByteInternal(getSystemCallMemory(address), address, data);
}

uint Core::coreIndex() const {return tile().coreIndex(id);}
uint Core::coresThisTile() const {return tile().numCores();}
uint Core::globalCoreIndex() const {return tile().globalCoreIndex(id);}

ComputeTile& Core::tile() const {
  return static_cast<ComputeTile&>(*(this->get_parent_object()));
}

ComponentID Core::getSystemCallMemory(MemoryAddr address) const {
  // If accessing a group of memories, determine which bank to access.
  // Default data memory channel is 1.
  const ChannelMapEntry& channelMap = channelMapTable.debugRead(1);

  uint increment = channelMap.computeAddressIncrement(address);
  return ComponentID(id.tile,
      channelMap.memoryView().bank + increment + coresThisTile());
}

} // end namespace

