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
    pipeReg1("pipe_reg1", PipelineRegister::FETCH_DECODE),
    pipeReg2("pipe_reg2", PipelineRegister::DECODE_EXECUTE),
    pipeReg3("pipe_reg3", PipelineRegister::EXECUTE_WRITE),
    forwarding("forward", params.registerFile.size),
    registers("regs", params.registerFile, params.numInputChannels - 2),
    scratchpad("scratchpad", params.scratchpad),
    channelMapTable("cmt", params.channelMapTable),
    controlRegisters("cregs"),
    predicate("predicate"),
    iInstFIFO("ipk_fifo", params.ipkFIFO),
    iInstCache("ipk_cache", params.cache),
    iDataFIFOs("iBuffers", params.numInputChannels - 2, params.inputFIFO),
    oDataFIFOs("oBuffers", params.outputFIFO),
    memoryBankSelector(ID),
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

  fetchStage.clock(clock);
  decodeStage.clock(clock);
  executeStage.clock(clock);
  writeStage.clock(clock);
  fetchStage.nextStage(pipeReg1); decodeStage.previousStage(pipeReg1);
  decodeStage.nextStage(pipeReg2); executeStage.previousStage(pipeReg2);
  executeStage.nextStage(pipeReg3); writeStage.previousStage(pipeReg3);

}

void Core::readRegister(DecodedInstruction inst, RegisterIndex index,
                        PortIndex port) {
  if (registers.isReadOnly(index))
    registers.read(inst, index, port);
  else if (registers.isFIFOMapped(index))
    iDataFIFOs.read(inst, index - 2, port);
  else if (forwarding.requiresForwarding(inst, port))
    forwarding.addConsumer(inst, port);
  else
    registers.read(inst, index, port);
}

void Core::writeRegister(DecodedInstruction inst, RegisterIndex index,
                         RegisterFile::write_t value) {
  loki_assert(!registers.isReadOnly(index));
  loki_assert(!registers.isFIFOMapped(index));

  registers.write(inst, index, value);
  // Could also remove this instruction from the forwarding network at this
  // point.
}

void Core::readPredicate(DecodedInstruction inst) {
  predicate.read(inst);
}
void Core::writePredicate(DecodedInstruction inst,
                          PredicateRegister::write_t value) {
  predicate.write(inst, value);
}

void Core::fetch(DecodedInstruction inst, MemoryAddr address,
                 ChannelMapEntry::MemoryChannel channel,
                 bool execute, bool persistent) {
  // TODO
}
void Core::jump(DecodedInstruction inst, JumpOffset offset) {
  // TODO
}

void Core::readCMT(DecodedInstruction inst, RegisterIndex index,
                   PortIndex port) {
  channelMapTable.read(inst, index, port);
}
void Core::writeCMT(DecodedInstruction inst, RegisterIndex index,
                    ChannelMapTable::write_t value) {
  channelMapTable.write(inst, index, value);
}

void Core::readCreg(DecodedInstruction inst, RegisterIndex index) {
  controlRegisters.read(inst, index);
}
void Core::writeCreg(DecodedInstruction inst, RegisterIndex index,
                     ControlRegisters::write_t value) {
  controlRegisters.write(inst, index, value);
}

void Core::readScratchpad(DecodedInstruction inst, RegisterIndex index) {
  scratchpad.read(inst, index);
}
void Core::writeScratchpad(DecodedInstruction inst, RegisterIndex index,
                           Scratchpad::write_t value) {
  scratchpad.write(inst, index, value);
}

void Core::syscall(DecodedInstruction inst, int code) {
  systemCallHandler.call((SystemCall::Code)code);

  // Instant execution. TODO: add latency to syscalls.
  inst->computeCallback(0); // Dummy result to satisfy interface.
}

void Core::selectChannelWithData(DecodedInstruction inst, uint bitmask) {
  iDataFIFOs.selectChannelWithData(inst, bitmask);
}

void Core::sendOnNetwork(DecodedInstruction inst, NetworkData flit) {
  oDataFIFOs.write(inst, flit);
}
void Core::waitForCredit(DecodedInstruction inst, ChannelIndex channel) {
  channelMapTable.waitForCredit(inst, channel);
}


uint Core::creditsAvailable(ChannelIndex channel) const {
  return channelMapTable.creditsAvailable(channel);
}
ChannelID Core::getNetworkDestination(EncodedCMTEntry channelMap,
                                      MemoryAddr address) const {
  return memoryBankSelector.getNetworkAddress(channelMap, address);
}

bool Core::inputFIFOHasData(ChannelIndex fifo) const {
  return iDataFIFOs.hasData(fifo);
}



RegisterFile::read_t Core::debugRegisterRead(RegisterIndex reg) {
  return registers.debugRead(reg);
}
void Core::debugRegisterWrite(RegisterIndex reg, RegisterFile::write_t data) {
  registers.debugWrite(reg, data);
}
int32_t Core::debugReadByte(MemoryAddr address) {
  return tile().readByteInternal(getSystemCallMemory(address), address).toInt();
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
  EncodedCMTEntry channelMap = channelMapTable.debugRead(1);
  ChannelID networkAddr = getNetworkDestination(channelMap, address);
  return networkAddr.component;
}

} // end namespace

