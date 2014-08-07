/*
 * Cluster.cpp
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
  for(unsigned int i=0; i<data.size(); i++) {
    instructions.push_back(static_cast<Instruction>(data[i]));
  }

  fetch.storeCode(instructions);
}

const MemoryAddr Core::getInstIndex() const   {return fetch.getInstIndex();}
bool     Core::canCheckTags() const           {return fetch.canCheckTags();}
bool     Core::readyToFetch() const           {return fetch.roomToFetch();}
void     Core::jump(const JumpOffset offset)  {fetch.jump(offset);}

bool     Core::inCache(const MemoryAddr addr, opcode_t operation) {
  return fetch.inCache(addr, operation);
}

const int32_t Core::readReg(PortIndex port, RegisterIndex reg, bool indirect) {

  int32_t result;

  // Register 0 is hard-wired to 0.
  if(reg == 0)
    return 0;

  // Perform data bypass if a register is being read and written in the same
  // cycle. This would usually happen in the register file, but it is a pain to
  // do it that way in SystemC.
  // Slightly complicated by the possibility of indirect register access - the
  // stated register may not actually be the one providing/receiving the data.
  if(reg == write.currentInstruction().destination() &&
     write.currentInstruction().opcode() != InstructionMap::OP_IWTR) {
    result = write.currentInstruction().result();

    // In a real system, we wouldn't know we were bypassing until too late, so
    // we have to perform the read anyway.
    regs.read(port, reg, false);

    if(DEBUG) cout << this->name() << " bypassed read of register "
                   << (int)reg << " (" << result << ")" << endl;
    Instrumentation::Registers::bypass(port);

    if(indirect) result = regs.read(port, result, false);
  }
  else result = regs.read(port, reg, indirect);

  return result;
}

/* Read a register without data forwarding and without indirection. */
const int32_t Core::readRegDebug(RegisterIndex reg) const {
  return regs.readDebug(reg);
}

void     Core::writeReg(RegisterIndex reg, int32_t value, bool indirect) {
  regs.write(reg, value, indirect);
}

bool     Core::readPredReg(bool waitForExecution) {
  // The wait parameter tells us to wait for the predicate to be written if
  // the instruction in the execute stage will set it.
  if(waitForExecution && execute.currentInstruction().setsPredicate()
                      && !execute.currentInstruction().hasResult()) {
    Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_FORWARDING);
    wait(execute.executedEvent());
    Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_FORWARDING);
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

void     Core::updateCurrentPacket(MemoryAddr addr) {
  regs.updateCurrentIPK(addr);
}

void     Core::pipelineStalled(bool stalled) {
  // Instrumentation is done at the source of the stall, so isn't needed here.
  currentlyStalled = stalled;
  stallEvent.notify();

  if(DEBUG)
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

void     Core::idlenessChanged() {
  // Use the decoder as the arbiter of idleness - we may sometimes bypass the
  // fetch stage.
  Instrumentation::idle(id, oIdle.read());
}

void Core::requestArbitration(ChannelID destination, bool request) {
  // Could have extra ports and write to them from here, but for the moment,
  // access the network directly.
  localNetwork->makeRequest(id, destination, request);
}

bool Core::requestGranted(ChannelID destination) const {
  return localNetwork->requestGranted(id, destination);
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
  stageIdle.init(4);
  stageReady.init(3);

  dataToBuffers.init(CORE_INPUT_CHANNELS);
  fcFromBuffers.init(CORE_INPUT_CHANNELS);
  dataConsumed.init(CORE_INPUT_CHANNELS);

  // Wire the input ports to the input buffers.
  for(unsigned int i=0; i<CORE_INPUT_PORTS; i++)
    inputCrossbar.iData[i](iData[i]);
  inputCrossbar.iData[CORE_INPUT_PORTS](iDataGlobal);

  for(unsigned int i=0; i<CORE_INPUT_CHANNELS; i++) {
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

  fetch.clock(clock);                     fetch.idle(stageIdle[0]);
  fetch.iToFIFO(dataToBuffers[0]);      fetch.oFlowControl[0](fcFromBuffers[0]);
  fetch.iToCache(dataToBuffers[1]);     fetch.oFlowControl[1](fcFromBuffers[1]);
  fetch.oDataConsumed[0](dataConsumed[0]); fetch.oDataConsumed[1](dataConsumed[1]);
  fetch.initPipeline(NULL, pipelineRegs[0]);

  decode.clock(clock);                    decode.idle(/*stageIdle[1]*/oIdle);
  decode.oReady(stageReady[0]);
  for(uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    decode.iData[i](dataToBuffers[i+2]);
    decode.oFlowControl[i](fcFromBuffers[i+2]);
    decode.oDataConsumed[i](dataConsumed[i+2]);
  }
  decode.initPipeline(pipelineRegs[0], pipelineRegs[1]);

  execute.clock(clock);                   execute.idle(stageIdle[2]);
  execute.oReady(stageReady[1]);
  execute.oData(outputData);            execute.iReady(stageReady[2]);
  execute.initPipeline(pipelineRegs[1], pipelineRegs[2]);

  write.clock(clock);                     write.idle(stageIdle[3]);
  write.oReady(stageReady[2]);
  write.iData(outputData);
  write.oDataLocal(oData[0]);
  write.oDataGlobal(oDataGlobal);
  write.iCredit(iCredit);
  write.initPipeline(pipelineRegs[2], NULL);

  // The core is considered idle if it did not decode a new instruction this
  // cycle. This is better than being controlled by the fetch stage because
  // some instructions take multiple cycles to decode, and because we may
  // bypass the fetch stage and receive instructions from other cores.
  // Problem: what about instructions that take multiple cycles to execute?
  SC_METHOD(idlenessChanged);
  sensitive << oIdle;
  // do initialise

  // Initialise the values in some wires.
  constantHigh.write(true);
}
