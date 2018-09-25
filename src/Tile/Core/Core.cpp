/*
 * Core.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Core.h"

#include "ControlRegisters.h"
#include "../ComputeTile.h"
#include "../../Datatype/DecodedInst.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/Registers.h"
#include "../../Utility/ISA.h"

/* Initialise the instructions a Core will execute. */
void     Core::storeData(const std::vector<Word>& data) {
  std::vector<Instruction> instructions;

  // Convert all of the words to instructions
  for (unsigned int i=0; i<data.size(); i++) {
    instructions.push_back(static_cast<Instruction>(data[i]));
  }

  fetch.storeCode(instructions);
}

MemoryAddr Core::getInstIndex() const   {return fetch.getInstAddress();}
bool     Core::canCheckTags() const           {return fetch.canCheckTags();}
void     Core::jump(const JumpOffset offset)  {fetch.jump(offset);}

void     Core::checkTags(MemoryAddr addr, opcode_t op, EncodedCMTEntry netInfo) {
  fetch.checkTags(addr, op, netInfo);
}

int32_t Core::readReg(PortIndex port, RegisterIndex reg, bool indirect) {

  int32_t result;

  // Register 0 is hard-wired to 0.
  if (reg == 0)
    return 0;

  // Perform data bypass if a register is being read and written in the same
  // cycle. This would usually happen in the register file, but it is a pain to
  // do it that way in SystemC.
  // Slightly complicated by the possibility of indirect register access - the
  // stated register may not actually be the one providing/receiving the data.
  if (reg == write.currentInstruction().destination() &&
      write.currentInstruction().opcode() != ISA::OP_IWTR) {
    result = write.currentInstruction().result();

    // In a real system, we wouldn't know we were bypassing until too late, so
    // we have to perform the read anyway.
    regs.read(port, reg, false);

    LOKI_LOG << this->name() << " bypassed read of register "
             << (int)reg << " (" << result << ")" << endl;
    Instrumentation::Registers::bypass(port);

    if (indirect) result = regs.read(port, result, false);
  }
  else result = regs.read(port, reg, indirect);

  return result;
}

/* Read a register without data forwarding and without indirection. */
int32_t Core::readRegDebug(RegisterIndex reg) const {
  if (regs.isChannelEnd(reg))
    return decode.readChannelInternal(regs.toChannelID(reg));
  else
    return regs.readInternal(reg);
}

void     Core::writeReg(RegisterIndex reg, int32_t value, bool indirect) {
  regs.write(reg, value, indirect);
}

bool     Core::readPredReg(bool waitForExecution, const DecodedInst& inst) {
  // The wait parameter tells us to wait for the predicate to be written if
  // the instruction in the execute stage will set it.
  if (waitForExecution && execute.currentInstruction().setsPredicate()) {
    Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_FORWARDING, inst);
    // Wait for at least one clock cycle.
    // FIXME: what if the execute stage's instruction finished on a previous cycle?
    // Then we don't want to wait at all.
    do
      wait(clock.posedge_event());
    while (!execute.currentInstruction().hasResult());
    Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_FORWARDING, inst);
  }

  return pred.read();
}

void     Core::writePredReg(bool val)  {pred.write(val);}

const Word Core::readWord(MemoryAddr addr) {
  return Word(parent().readWordInternal(getSystemCallMemory(addr), addr));
}

const Word Core::readByte(MemoryAddr addr) {
  return Word(parent().readByteInternal(getSystemCallMemory(addr), addr));
}

void Core::writeWord(MemoryAddr addr, Word data) {
  parent().writeWordInternal(getSystemCallMemory(addr), addr, data);
}

void Core::writeByte(MemoryAddr addr, Word data) {
  parent().writeByteInternal(getSystemCallMemory(addr), addr, data);
}

void Core::magicMemoryAccess(const DecodedInst& instruction) {
  magicMemoryConnection.operate(instruction);
}

void Core::magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address, ChannelID returnChannel, Word payload) {
  parent().magicMemoryAccess(opcode, address, returnChannel, payload);
}

void Core::deliverDataInternal(const NetworkData& flit) {
  switch (flit.channelID().channel) {
    case 0:
    case 1:
      fetch.deliverInstructionInternal(flit);
      break;

    default:
      decode.deliverDataInternal(flit);
      break;
  }
}

void Core::deliverCreditInternal(const NetworkCredit& flit) {
  write.deliverCreditInternal(flit);
}

size_t Core::numInputDataBuffers() const {
  // A little hacky. The alternative is to ask the DecodeStage to ask the
  // ReceiveChannelEndTable to ask its BufferArray how big it is.
  return dataToBuffers.size() - numInstructionChannels;
}

uint Core::coreIndex() const {
  return parent().coreIndex(id);
}

uint Core::coresThisTile() const {
  return parent().numCores();
}

uint Core::globalCoreIndex() const {
  return parent().globalCoreIndex(id);
}

bool Core::isCore(ComponentID id) const {
  return parent().isCore(id);
}

bool Core::isMemory(ComponentID id) const {
  return parent().isMemory(id);
}

bool Core::isComputeTile(TileID id) const {
  return parent().isComputeTile(id);
}

int32_t  Core::readRCET(ChannelIndex channel) {
  return decode.readChannel(channel);
}

int32_t  Core::getForwardedData() const {
  loki_assert(execute.currentInstruction().hasResult());
  return execute.currentInstruction().result();
}

void     Core::updateCurrentPacket(MemoryAddr addr) {
  regs.updateCurrentIPK(addr);
}

void     Core::updateFetchAddressCReg(MemoryAddr addr) {
  cregs.write(ControlRegisters::CR_FETCH_ADDRESS, addr);
}

void     Core::pipelineStalled(bool stalled) {
  // Instrumentation is done at the source of the stall, so isn't needed here.
  currentlyStalled = stalled;
  stallEvent.notify();

  LOKI_LOG << this->name() << ": pipeline " << (stalled ? "stalled" : "unstalled") << endl;
}

bool     Core::discardInstruction(int stage) {
  // The first pipeline stage to have a stall register before it is the 2nd.
  // Therefore reduce the index by 2 to get the position in the array.
  return pipelineRegs[stage-2].discard();
}

void     Core::nextIPK() {
  // If we are stalled waiting for any input, unstall.
  decode.unstall();

  // Discard any instructions which were queued up behind any stalled stages.
  while(pipelineRegs[0].discard())
    /* continue discarding */;
}

void     Core::idle(bool state) {
  // Use the decoder as the arbiter of idleness - we may sometimes bypass the
  // fetch stage.
  Instrumentation::idle(id, state);
}

void Core::requestArbitration(ChannelID destination, bool request) {
  // Could have extra ports and write to them from here, but for the moment,
  // access the network directly.
  parent().makeRequest(id, destination, request);
}

bool Core::requestGranted(ChannelID destination) const {
  return parent().requestGranted(id, destination);
}

ComputeTile& Core::parent() const {
  return static_cast<ComputeTile&>(*(this->get_parent_object()));
}

void Core::trace(const DecodedInst& inst) const {
  // For every instruction executed, print:
  //  * The core it executed on
  //  * The instruction and all of its operands
  //  * Any core state (register contents, predicate register, etc.)
  // Code is borrowed from CSIM to ensure that the format is the same.

  char regbuf[512];
  int i;
  char *bufpos = regbuf;
  for (i=0; i<32; i++) {
    if (i > 0)
      *bufpos++ = ',';
    bufpos += snprintf(bufpos, 512-(bufpos-regbuf), "%d", readRegDebug(i));
  }

  printf("CPU%d 0x%08x: %s %d %d %d %d %d %d p=%d, regs={%s}\n",
      parent().globalCoreIndex(id), inst.location(), inst.name().c_str(),
      inst.destination(), inst.sourceReg1(), inst.sourceReg2(),
      inst.immediate(), inst.immediate2(), inst.channelMapEntry(),
      pred.read(), regbuf);
}

ComponentID Core::getSystemCallMemory(MemoryAddr address) {
  // If accessing a group of memories, determine which bank to access.
  uint increment = channelMapTable[1].computeAddressIncrement(address);
  return ComponentID(id.tile,
      channelMapTable[1].memoryView().bank + increment + coresThisTile());
}

/* Returns the channel ID of this core's instruction packet FIFO. */
ChannelID Core::IPKFIFOInput(const ComponentID& ID) {
  return ChannelID(ID, 0);
}

/* Returns the channel ID of this core's instruction packet cache. */
ChannelID Core::IPKCacheInput(const ComponentID& ID) {
  return ChannelID(ID, 1);
}

/* Returns the channel ID of this core's specified input channel. */
ChannelID Core::RCETInput(const ComponentID& ID, ChannelIndex channel) {
  return ChannelID(ID, 2 + channel);
}

Core::Core(const sc_module_name& name, const ComponentID& ID,
           const core_parameters_t& params, size_t numMulticastInputs) :
    LokiComponent(name, ID),
    clock("clock"),
    iInstruction("iInstruction"),
    iData("iData"),
    oRequest("oRequest"),
    iMulticast("iMulticast", numMulticastInputs),
    oMulticast("oMulticast"),
    oDataGlobal("oDataGlobal"),
    iDataGlobal("iDataGlobal"),
    oReadyData("oReadyData", params.numInputChannels),
    oCredit("oCredit"),
    iCredit("iCredit"),
    oReadyCredit("oReadyCredit"),
    fastClock("fastClock"),
    inputCrossbar("input_crossbar", ID, numMulticastInputs+3, params.numInputChannels), // cores + insts + data + router
    regs("regs", ID, params.registerFile),
    pred("predicate"),
    fetch("fetch", ID, params.ipkFIFO, params.cache),
    decode("decode", ID, params.numInputChannels-numInstructionChannels, params.inputFIFO),
    execute("execute", ID, params.scratchpad),
    write("write", ID, params.outputFIFO),
    channelMapTable("channel_map_table", ID, params.channelMapTable, params.numInputChannels),
    cregs("cregs", ID),
    magicMemoryConnection("magic_memory", ID),
    stageReady("stageReady", 3), // 4 stages => 3 links between stages
    dataToBuffers("dataToBuffers", params.numInputChannels),
    fcFromBuffers("fcFromBuffers", params.numInputChannels),
    dataConsumed("dataConsumed", params.numInputChannels) {

  currentlyStalled = false;

  oReadyCredit.initialize(true);

  // Wire the input ports to the input buffers.
  inputCrossbar.iData[0](iInstruction);
  inputCrossbar.iData[1](iData);
  inputCrossbar.iData[2](iDataGlobal);
  for (unsigned int i=3; i<3+iMulticast.size(); i++)
    inputCrossbar.iData[i](iMulticast[i-3]);

  inputCrossbar.oReady(oReadyData);
  inputCrossbar.oData(dataToBuffers);
  inputCrossbar.iFlowControl(fcFromBuffers);
  inputCrossbar.iDataConsumed(dataConsumed);
  inputCrossbar.clock(clock);
  inputCrossbar.creditClock(fastClock);
  inputCrossbar.oCredit[0](oCredit);

  cregs.clock(clock);

  // Create pipeline registers.
  pipelineRegs.push_back(
      new PipelineRegister(sc_gen_unique_name("pipe_reg"), id, PipelineRegister::FETCH_DECODE));
  pipelineRegs.push_back(
      new PipelineRegister(sc_gen_unique_name("pipe_reg"), id, PipelineRegister::DECODE_EXECUTE));
  pipelineRegs.push_back(
      new PipelineRegister(sc_gen_unique_name("pipe_reg"), id, PipelineRegister::EXECUTE_WRITE));

  // Wire the pipeline stages up.

  fetch.clock(clock);
  fetch.iToFIFO(dataToBuffers[0]);          fetch.oFlowControl[0](fcFromBuffers[0]);
  fetch.iToCache(dataToBuffers[1]);         fetch.oFlowControl[1](fcFromBuffers[1]);
  fetch.oDataConsumed[0](dataConsumed[0]);  fetch.oDataConsumed[1](dataConsumed[1]);
  fetch.oFetchRequest(fetchFlitSignal);
  fetch.iOutputBufferReady(stageReady[2]);
  fetch.initPipeline(NULL, &pipelineRegs[0]);

  decode.clock(clock);
  decode.oReady(stageReady[0]);
  decode.iOutputBufferReady(stageReady[2]);
  for (uint i=numInstructionChannels; i<params.numInputChannels; i++) {
    decode.iData[i-numInstructionChannels](dataToBuffers[i]);
    decode.oFlowControl[i-numInstructionChannels](fcFromBuffers[i]);
    decode.oDataConsumed[i-numInstructionChannels](dataConsumed[i]);
  }
  decode.initPipeline(&pipelineRegs[0], &pipelineRegs[1]);

  execute.clock(clock);
  execute.oReady(stageReady[1]);
  execute.oData(dataFlitSignal);            execute.iReady(stageReady[2]);
  execute.initPipeline(&pipelineRegs[1], &pipelineRegs[2]);

  write.clock(clock);
  write.oReady(stageReady[2]);
  write.iFetch(fetchFlitSignal);
  write.iData(dataFlitSignal);
  write.oDataMemory(oRequest);
  write.oDataLocal(oMulticast);
  write.oDataGlobal(oDataGlobal);
  write.iCredit(iCredit);
  write.initPipeline(&pipelineRegs[2], NULL);
}
