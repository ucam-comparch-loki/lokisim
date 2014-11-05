/*
 * Core.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "../Chip.h"
#include "Core.h"
#include "Pipeline/PipelineRegister.h"
#include "../Datatype/ChannelID.h"
#include "../Datatype/DecodedInst.h"
#include "../Network/Topologies/LocalNetwork.h"
#include "../Utility/InstructionMap.h"
#include "../Utility/Instrumentation/Registers.h"

/* Initialise the instructions a Cluster will execute. */
void     Core::storeData(const std::vector<Word>& data, MemoryAddr location) {
  std::vector<Instruction> instructions;

  // Convert all of the words to instructions
  for (unsigned int i=0; i<data.size(); i++) {
    instructions.push_back(static_cast<Instruction>(data[i]));
  }

  fetch.storeCode(instructions);
}

const MemoryAddr Core::getInstIndex() const   {return fetch.getInstAddress();}
bool     Core::canCheckTags() const           {return fetch.canCheckTags();}
void     Core::jump(const JumpOffset offset)  {fetch.jump(offset);}

void     Core::checkTags(MemoryAddr addr, opcode_t op, ChannelID channel, ChannelIndex returnChannel) {
  fetch.checkTags(addr, op, channel, returnChannel);
}

const int32_t Core::readReg(PortIndex port, RegisterIndex reg, bool indirect) {

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
      write.currentInstruction().opcode() != InstructionMap::OP_IWTR) {
    result = write.currentInstruction().result();

    // In a real system, we wouldn't know we were bypassing until too late, so
    // we have to perform the read anyway.
    regs.read(port, reg, false);

    if (DEBUG) cout << this->name() << " bypassed read of register "
                    << (int)reg << " (" << result << ")" << endl;
    Instrumentation::Registers::bypass(port);

    if (indirect) result = regs.read(port, result, false);
  }
  else result = regs.read(port, reg, indirect);

  return result;
}

/* Read a register without data forwarding and without indirection. */
const int32_t Core::readRegDebug(RegisterIndex reg) const {
  if (RegisterFile::isChannelEnd(reg))
    return decode.readRCETDebug(RegisterFile::toChannelID(reg));
  else
    return regs.readDebug(reg);
}

void     Core::writeReg(RegisterIndex reg, int32_t value, bool indirect) {
  regs.write(reg, value, indirect);
}

bool     Core::readPredReg(bool waitForExecution, const DecodedInst& inst) {
  // The wait parameter tells us to wait for the predicate to be written if
  // the instruction in the execute stage will set it.
  if (waitForExecution && execute.currentInstruction().setsPredicate()
                       && !execute.currentInstruction().hasResult()) {
    Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_FORWARDING, inst);
    wait(execute.executedEvent());
    Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_FORWARDING, inst);
  }

  return pred.read();
}

void     Core::writePredReg(bool val)  {pred.write(val);}

const Word Core::readWord(MemoryAddr addr) const {
	return Word(parent()->readWord(getSystemCallMemory(), addr));
}

const Word Core::readByte(MemoryAddr addr) const {
	return Word(parent()->readByte(getSystemCallMemory(), addr));
}

void Core::writeWord(MemoryAddr addr, Word data) {
	parent()->writeWord(getSystemCallMemory(), addr, data);
}

void Core::writeByte(MemoryAddr addr, Word data) {
	parent()->writeByte(getSystemCallMemory(), addr, data);
}

const int32_t  Core::readRCET(ChannelIndex channel) {
  return decode.readRCET(channel);
}

const int32_t  Core::getForwardedData() const {
  assert(execute.currentInstruction().hasResult());
  return execute.currentInstruction().result();
}

void     Core::updateCurrentPacket(MemoryAddr addr) {
  regs.updateCurrentIPK(addr);
}

void     Core::pipelineStalled(bool stalled) {
  // Instrumentation is done at the source of the stall, so isn't needed here.
  currentlyStalled = stalled;
  stallEvent.notify();

  if (DEBUG)
    cout << this->name() << ": pipeline " << (stalled ? "stalled" : "unstalled") << endl;
}

bool     Core::discardInstruction(int stage) {
  // The first pipeline stage to have a stall register before it is the 2nd.
  // Therefore reduce the index by 2 to get the position in the array.
  return pipelineRegs[stage-2]->discard();
}

void     Core::nextIPK() {
  // If we are stalled waiting for any input, unstall.
  decode.unstall();

  // Discard any instructions which were queued up behind any stalled stages.
  while(pipelineRegs[0]->discard())
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
  localNetwork->makeRequest(id, destination, request);
}

bool Core::requestGranted(ChannelID destination) const {
  return localNetwork->requestGranted(id, destination);
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
      id.getGlobalCoreNumber(), inst.location(), inst.name().c_str(),
      inst.destination(), inst.sourceReg1(), inst.sourceReg2(),
      inst.immediate(), inst.immediate2(), inst.channelMapEntry(),
      pred.read(), regbuf);
}

ComponentID Core::getSystemCallMemory() const {
  // TODO: Stop assuming that the first channel map entry after the fetch
  // channel corresponds to the memory that system calls want to access.
  return channelMapTable.read(1).getComponentID();
}

/* Returns the channel ID of this cluster's instruction packet FIFO. */
ChannelID Core::IPKFIFOInput(const ComponentID& ID) {
  return ChannelID(ID, 0);
}

/* Returns the channel ID of this cluster's instruction packet cache. */
ChannelID Core::IPKCacheInput(const ComponentID& ID) {
  return ChannelID(ID, 1);
}

/* Returns the channel ID of this cluster's specified input channel. */
ChannelID Core::RCETInput(const ComponentID& ID, ChannelIndex channel) {
  return ChannelID(ID, 2 + channel);
}

Core::Core(const sc_module_name& name, const ComponentID& ID, local_net_t* network) :
    TileComponent(name, ID, CORE_INPUT_PORTS, CORE_OUTPUT_PORTS),
    inputCrossbar("input_crossbar", ID),
    regs("regs", ID),
    pred("predicate"),
    fetch("fetch", ID),
    decode("decode", ID),
    execute("execute", ID),
    write("write", ID),
    channelMapTable("channel_map_table", ID),
    localNetwork(network) {

  currentlyStalled = false;

  oReadyData.init(CORE_INPUT_CHANNELS);
  oReadyCredit.initialize(true);

  // Create signals which connect the pipeline stages together. There are 4
  // stages, so 3 links between stages.
  stageReady.init(3);

  dataToBuffers.init(CORE_INPUT_CHANNELS);
  fcFromBuffers.init(CORE_INPUT_CHANNELS);
  dataConsumed.init(CORE_INPUT_CHANNELS);

  // Wire the input ports to the input buffers.
  for (unsigned int i=0; i<CORE_INPUT_PORTS; i++)
    inputCrossbar.iData[i](iData[i]);
  inputCrossbar.iData[CORE_INPUT_PORTS](iDataGlobal);

  for (unsigned int i=0; i<CORE_INPUT_CHANNELS; i++) {
    inputCrossbar.oReady[i](oReadyData[i]);
    inputCrossbar.oData[i](dataToBuffers[i]);
    inputCrossbar.iFlowControl[i](fcFromBuffers[i]);
    inputCrossbar.iDataConsumed[i](dataConsumed[i]);
  }

  inputCrossbar.clock(clock);
  inputCrossbar.creditClock(fastClock);
  inputCrossbar.oCredit[0](oCredit);

  // Create pipeline registers.
  pipelineRegs.push_back(
      new PipelineRegister(sc_gen_unique_name("pipe_reg"), id, PipelineRegister::FETCH_DECODE));
  pipelineRegs.push_back(
      new PipelineRegister(sc_gen_unique_name("pipe_reg"), id, PipelineRegister::DECODE_EXECUTE));
  pipelineRegs.push_back(
      new PipelineRegister(sc_gen_unique_name("pipe_reg"), id, PipelineRegister::EXECUTE_WRITE));

  // Wire the pipeline stages up.

  fetch.clock(clock);
  fetch.iToFIFO(dataToBuffers[0]);      fetch.oFlowControl[0](fcFromBuffers[0]);
  fetch.iToCache(dataToBuffers[1]);     fetch.oFlowControl[1](fcFromBuffers[1]);
  fetch.oDataConsumed[0](dataConsumed[0]); fetch.oDataConsumed[1](dataConsumed[1]);
  fetch.oFetchRequest(fetchFlitSignal);
  fetch.initPipeline(NULL, pipelineRegs[0]);

  decode.clock(clock);
  decode.oReady(stageReady[0]);
  for (uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    decode.iData[i](dataToBuffers[i+2]);
    decode.oFlowControl[i](fcFromBuffers[i+2]);
    decode.oDataConsumed[i](dataConsumed[i+2]);
  }
  decode.initPipeline(pipelineRegs[0], pipelineRegs[1]);

  execute.clock(clock);
  execute.oReady(stageReady[1]);
  execute.oData(dataFlitSignal);            execute.iReady(stageReady[2]);
  execute.initPipeline(pipelineRegs[1], pipelineRegs[2]);

  write.clock(clock);
  write.oReady(stageReady[2]);
  write.iFetch(fetchFlitSignal);
  write.iData(dataFlitSignal);
  write.oDataLocal(oData[0]);
  write.oDataGlobal(oDataGlobal);
  write.iCredit(iCredit);
  write.initPipeline(pipelineRegs[2], NULL);

  // Initialise the values in some wires.
  constantHigh.write(true);
}
