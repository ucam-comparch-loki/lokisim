/*
 * Cluster.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "../Chip.h"
#include "Cluster.h"
#include "InputCrossbar.h"
#include "Pipeline/StallRegister.h"
#include "../Utility/InstructionMap.h"
#include "../Datatype/ChannelID.h"
#include "../Datatype/DecodedInst.h"

/* Initialise the instructions a Cluster will execute. */
void     Cluster::storeData(const std::vector<Word>& data, MemoryAddr location) {
  std::vector<Instruction> instructions;

  // Convert all of the words to instructions
  for(uint i=0; i<data.size(); i++) {
    if(BYTES_PER_INSTRUCTION > BYTES_PER_WORD) {
      // Need to load multiple words to create each instruction.
      uint32_t first = (uint32_t)data[i].toInt();
      uint32_t second = (uint32_t)data[i+1].toInt();
      uint64_t total = ((uint64_t)first << 32) + second;
      instructions.push_back(Instruction(total));
      i++;
    }
    else {
      instructions.push_back(static_cast<Instruction>(data[i]));
    }
  }

  fetch.storeCode(instructions);
}

const MemoryAddr Cluster::getInstIndex() const   {return fetch.getInstIndex();}
bool     Cluster::inCache(const MemoryAddr a)    {return fetch.inCache(a);}
bool     Cluster::roomToFetch() const            {return fetch.roomToFetch();}
void     Cluster::refetch()                      {decode.refetch();}
void     Cluster::jump(const JumpOffset offset)  {fetch.jump(offset);}
void     Cluster::setPersistent(bool persistent) {fetch.setPersistent(persistent);}

const int32_t Cluster::readReg(RegisterIndex reg, bool indirect) const {

  int32_t result;

  // Check to see if the requested register is one of the previous two
  // registers written to. If so, we forward the data because the register
  // might not have been written to yet.
  // Slightly complicated by the possibility of indirect register reads.
  if(reg>0) {
    if(reg == previousDest1) {
      if(DEBUG) cout << this->name() << " forwarded contents of register "
                     << (int)reg << " (" << previousResult1 << ")" << endl;

      if(indirect) result = regs.read(previousResult1, false);
      else         result = previousResult1;
    }
    else if(reg == previousDest2) {
      if(DEBUG) cout << this->name() << " forwarded contents of register "
                     << (int)reg << " (" << previousResult2 << ")" << endl;

      if(indirect) result = regs.read(previousResult2, false);
      else         result = previousResult2;
    }
    else result = regs.read(reg, indirect);
  }
  else result = regs.read(reg, indirect);

  return result;
}

/* Read a register without data forwarding and without indirection. */
const int32_t Cluster::readRegDebug(RegisterIndex reg) const {
  return regs.readDebug(reg);
}

void     Cluster::writeReg(RegisterIndex reg, int32_t value, bool indirect) {
  regs.write(reg, value, indirect);
}

void     Cluster::checkForwarding(DecodedInst& inst) const {
  // We don't want to use forwarded data if we read from register 0: this could
  // mean that there is just no register specified, and we are using an
  // immediate instead.
  if(inst.sourceReg1() > 0) {
    if(inst.sourceReg1() == previousDest1) {
      inst.operand1(previousResult1);
      Instrumentation::dataForwarded(id, previousDest1);
    }
    else if(inst.sourceReg1() == previousDest2){
      inst.operand1(previousResult2);
      Instrumentation::dataForwarded(id, previousDest2);
    }
  }
  if(inst.sourceReg2() > 0) {
    if(inst.sourceReg2() == previousDest1) {
      inst.operand2(previousResult1);
      Instrumentation::dataForwarded(id, previousDest1);
    }
    else if(inst.sourceReg2() == previousDest2) {
      inst.operand2(previousResult2);
      Instrumentation::dataForwarded(id, previousDest2);
    }
  }
}

void     Cluster::updateForwarding(const DecodedInst& inst) {
  // Should this update happen every cycle, or every time the forwarding is
  // updated?
  previousDest2   = previousDest1;
  previousResult2 = previousResult1;
  previousDest1   = -1;

  if(!InstructionMap::storesResult(inst.operation())) return;

  // We don't want to forward any data which was sent to register 0, because
  // r0 doesn't store values: it is a constant.
  // We also don't want to forward data after an indirect write, as the data
  // won't have been stored in the destination register -- it will have been
  // stored in the register the destination register was pointing to.
  previousDest1   = (inst.destination() == 0 ||
                     inst.operation() == InstructionMap::IWTR)
                  ? -1 : inst.destination();
  previousResult1 = inst.result();
}

bool     Cluster::readPredReg() const     {return pred.read();}
void     Cluster::writePredReg(bool val)  {pred.write(val);}

const Word Cluster::readWord(MemoryAddr addr) const {
	return Word(parent()->readWord(write.getSystemCallMemory(), addr));
}

const Word Cluster::readByte(MemoryAddr addr) const {
	return Word(parent()->readByte(write.getSystemCallMemory(), addr));
}

void Cluster::writeWord(MemoryAddr addr, Word data) {
	parent()->writeWord(write.getSystemCallMemory(), addr, data);
}

void Cluster::writeByte(MemoryAddr addr, Word data) {
	parent()->writeByte(write.getSystemCallMemory(), addr, data);
}

const int32_t  Cluster::readRCET(ChannelIndex channel) {
  return decode.readRCET(channel);
}

void     Cluster::updateCurrentPacket(MemoryAddr addr) {
  regs.updateCurrentIPK(addr);
}

void     Cluster::pipelineStalled(bool stalled) {
  // Instrumentation is done at the source of the stall, so isn't needed here.
  currentlyStalled = stalled;

  if(DEBUG) {
    cout << this->name() << ": pipeline " << (stalled ? "stalled" : "unstalled") << endl;
  }
}

bool     Cluster::discardInstruction(int stage) {
  // The first pipeline stage to have a stall register before it is the 2nd.
  // Therefore reduce the index by 2 to get the position in the array.
  return stallRegs[stage-2]->discard();
}

void     Cluster::updateIdle() {
  bool wasIdle = idle.read();

  bool isIdle = true;
  for(uint i=0; i<4; i++) {
    if(!stageIdle[i].read()) {
      isIdle = false;
      break;
    }
  }

  // Is this what we really want?
  idle.write(isIdle || currentlyStalled);

  if(wasIdle != isIdle) Instrumentation::idle(id, isIdle);
}

/* Returns the channel ID of this cluster's instruction packet FIFO. */
ChannelID Cluster::IPKFIFOInput(const ComponentID& ID) {
  return ChannelID(ID, 0);
}

/* Returns the channel ID of this cluster's instruction packet cache. */
ChannelID Cluster::IPKCacheInput(const ComponentID& ID) {
  return ChannelID(ID, 1);
}

/* Returns the channel ID of this cluster's specified input channel. */
ChannelID Cluster::RCETInput(const ComponentID& ID, ChannelIndex channel) {
  return ChannelID(ID, 2 + channel);
}

Cluster::Cluster(sc_module_name name, const ComponentID& ID) :
    TileComponent(name, ID, CORE_INPUT_PORTS, CORE_OUTPUT_PORTS),
    regs("regs", ID),
    pred("predicate"),
    fetch("fetch", ID),
    decode("decode", ID),
    execute("execute", ID),
    write("write", ID) {

  previousDest1 = previousDest2 = -1;

  inputCrossbar = new InputCrossbar("input", ID, CORE_INPUT_PORTS, CORE_INPUT_CHANNELS);

  // Create signals, with the number based on the length of the pipeline.
  stageIdle     = new sc_signal<bool>[4];
  stallRegReady = new sc_signal<bool>[3];
  stageReady    = new sc_signal<bool>[3];
  dataToStage   = new flag_signal<DecodedInst>[3];
  dataFromStage = new sc_buffer<DecodedInst>[3];

  fcFromBuffers = new sc_signal<bool>[CORE_INPUT_CHANNELS];
  dataToBuffers = new sc_buffer<Word>[CORE_INPUT_CHANNELS];

  // Wire the input ports to the input buffers.
  for(uint i=0; i<CORE_INPUT_PORTS; i++) {
    inputCrossbar->dataIn[i](dataIn[i]);
    inputCrossbar->validDataIn[i](validDataIn[i]);
    inputCrossbar->ackDataIn[i](ackDataIn[i]);

    inputCrossbar->creditsOut[i](creditsOut[i]);
    inputCrossbar->validCreditOut[i](validCreditOut[i]);
    inputCrossbar->ackCreditOut[i](ackCreditOut[i]);
  }

  for(uint i=0; i<CORE_INPUT_CHANNELS; i++) {
    inputCrossbar->dataOut[i](dataToBuffers[i]);
    inputCrossbar->bufferHasSpace[i](fcFromBuffers[i]);
  }

  inputCrossbar->clock(clock);

  // Wire the stall registers up.
  for(uint i=0; i<3; i++) {
    StallRegister* stallReg = new StallRegister(sc_core::sc_gen_unique_name("stall"), i);

    stallReg->clock(clock);               stallReg->readyOut(stallRegReady[i]);
    stallReg->dataIn(dataFromStage[i]);   stallReg->dataOut(dataToStage[i]);
    stallReg->localStageReady(stageReady[i]);

    // The final stall register gets a different ready signal.
    if(i < 2) stallReg->readyIn(stallRegReady[i+1]);

    stallRegs.push_back(stallReg);
  }
  stallRegs.back()->readyIn(constantHigh);

  // Wire the pipeline stages up.

  fetch.clock(clock);                     fetch.idle(stageIdle[0]);
  fetch.toIPKFIFO(dataToBuffers[0]);      fetch.flowControl[0](fcFromBuffers[0]);
  fetch.toIPKCache(dataToBuffers[1]);     fetch.flowControl[1](fcFromBuffers[1]);
  fetch.readyIn(stallRegReady[0]);        fetch.dataOut(dataFromStage[0]);

  decode.clock(clock);                    decode.idle(stageIdle[1]);
  decode.fetchOut(fetchSignal);
  decode.flowControlIn(stageReady[2]); // Flow control from output buffers
  decode.readyIn(stallRegReady[1]);       decode.dataOut(dataFromStage[1]);
  decode.readyOut(stageReady[0]);         decode.dataIn(dataToStage[0]);
  for(uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    decode.rcetIn[i](dataToBuffers[i+2]);
    decode.flowControlOut[i](fcFromBuffers[i+2]);
  }

  execute.clock(clock);                   execute.idle(stageIdle[2]);
  execute.readyOut(stageReady[1]);        execute.dataIn(dataToStage[1]);
  execute.dataOut(dataFromStage[2]);

  write.clock(clock);                     write.idle(stageIdle[3]);
  write.readyOut(stageReady[2]);          write.dataIn(dataToStage[2]);
  write.fromFetchLogic(fetchSignal);

  for(unsigned int i=0; i<CORE_OUTPUT_PORTS; i++) {
    write.output[i](dataOut[i]);            write.creditsIn[i](creditsIn[i]);
    write.validOutput[i](validDataOut[i]);  write.validCredit[i](validCreditIn[i]);
    write.ackOutput[i](ackDataOut[i]);
  }

  SC_METHOD(updateIdle);
  for(uint i=0; i<4; i++) sensitive << stageIdle[i];
  // do initialise

  // Initialise the values in some wires.
  idle.initialize(true);
  constantHigh.write(true);
}

Cluster::~Cluster() {
  delete inputCrossbar;

  delete[] stageIdle;
  delete[] stallRegReady;
  delete[] stageReady;
  delete[] dataToStage;
  delete[] dataFromStage;

  for(uint i=0; i<stallRegs.size(); i++) delete stallRegs[i];
}
