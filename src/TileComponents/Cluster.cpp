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
#include "Pipeline/Fetch/FetchStage.h"
#include "Pipeline/Decode/DecodeStage.h"
#include "Pipeline/Execute/ExecuteStage.h"
#include "Pipeline/Write/WriteStage.h"
#include "../Utility/InstructionMap.h"
#include "../Datatype/ChannelID.h"
#include "../Datatype/DecodedInst.h"

double   Cluster::area() const {
  return regs.area()   + pred.area();//    + fetch.area() +
         //decode.area() + execute.area() + write.area();
}

double   Cluster::energy() const {
  return regs.energy()   + pred.energy();//    + fetch.energy() +
         //decode.energy() + execute.energy() + write.energy();
}

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

  FetchStage*  fetch  = dynamic_cast<FetchStage*>(stages.front());
  fetch->storeCode(instructions);
}

const MemoryAddr Cluster::getInstIndex() const {
  FetchStage*  fetch  = dynamic_cast<FetchStage*>(stages.front());
  return fetch->getInstIndex();
}

bool     Cluster::inCache(const MemoryAddr a) {
  FetchStage*  fetch  = dynamic_cast<FetchStage*>(stages.front());
  return fetch->inCache(a);
}

bool     Cluster::roomToFetch() const {
  FetchStage*  fetch  = dynamic_cast<FetchStage*>(stages.front());
  return fetch->roomToFetch();
}

void     Cluster::refetch() {
  DecodeStage* decode = dynamic_cast<DecodeStage*>(stages[1]);
  decode->refetch();
}

void     Cluster::jump(const JumpOffset offset) {
  FetchStage*  fetch  = dynamic_cast<FetchStage*>(stages.front());
  fetch->jump(offset);
}

void     Cluster::setPersistent(bool persistent) {
  FetchStage*  fetch  = dynamic_cast<FetchStage*>(stages.front());
  fetch->setPersistent(persistent);
}

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

bool     Cluster::readPredReg() const {
  return pred.read();
}

const Word Cluster::readWord(MemoryAddr addr) const {
	WriteStage* writeStage = dynamic_cast<WriteStage*>(stages.back());
	return Word(parent()->readWord(writeStage->getSystemCallMemory(), addr));
}

const Word Cluster::readByte(MemoryAddr addr) const {
	WriteStage* writeStage = dynamic_cast<WriteStage*>(stages.back());
	return Word(parent()->readByte(writeStage->getSystemCallMemory(), addr));
}

void Cluster::writeWord(MemoryAddr addr, Word data) {
	WriteStage* writeStage = dynamic_cast<WriteStage*>(stages.back());
	parent()->writeWord(writeStage->getSystemCallMemory(), addr, data);
}

void Cluster::writeByte(MemoryAddr addr, Word data) {
	WriteStage* writeStage = dynamic_cast<WriteStage*>(stages.back());
	parent()->writeByte(writeStage->getSystemCallMemory(), addr, data);
}

void     Cluster::writePredReg(bool val) {
  pred.write(val);
}

const int32_t  Cluster::readRCET(ChannelIndex channel) {
  DecodeStage* decode = dynamic_cast<DecodeStage*>(stages[1]);
  return decode->readRCET(channel);
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
  constantHigh.write(true);   // Hack: find a better way of doing this.

  bool isIdle = true;
  for(uint i=0; i<stages.size(); i++) {
    if(!stageIdle[i].read()) {
      isIdle = false;
      break;
    }
  }

  // Is this what we really want?
  idle.write(isIdle || currentlyStalled);

  Instrumentation::idle(id, isIdle);
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
    pred("predicate") {

  previousDest1 = previousDest2 = -1;

  inputCrossbar = new InputCrossbar("input", ID, CORE_INPUT_PORTS, CORE_INPUT_CHANNELS);

  // Create pipeline stages and put into a vector (allows arbitrary length
  // pipeline).
  stages.push_back(new FetchStage("fetch", ID));
  stages.push_back(new DecodeStage("decode", ID));
  stages.push_back(new ExecuteStage("execute", ID));
  stages.push_back(new WriteStage("write", ID));

  // Create signals, with the number based on the length of the pipeline.
  stageIdle     = new sc_signal<bool>[stages.size()];
  stallRegReady = new sc_signal<bool>[stages.size()-1];
  stageReady    = new sc_signal<bool>[stages.size()-1];
  dataToStage   = new flag_signal<DecodedInst>[stages.size()-1];
  dataFromStage = new sc_buffer<DecodedInst>[stages.size()-1];

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
  for(uint i=0; i<stages.size()-1; i++) {
    StallRegister* stallReg = new StallRegister(sc_core::sc_gen_unique_name("stall"), i);

    stallReg->clock(clock);               stallReg->readyOut(stallRegReady[i]);
    stallReg->dataIn(dataFromStage[i]);   stallReg->dataOut(dataToStage[i]);
    stallReg->localStageReady(stageReady[i]);

    // The final stall register gets a different ready signal.
    if(i<stages.size()-2) stallReg->readyIn(stallRegReady[i+1]);

    stallRegs.push_back(stallReg);
  }
  stallRegs.back()->readyIn(constantHigh);

  // Wire the pipeline stages up.
  for(uint i=0; i<stages.size(); i++) {
    stages[i]->clock(clock);              stages[i]->idle(stageIdle[i]);

    // All stages except the first have some ports.
    if(i>0) {
      StageWithPredecessor* stage = dynamic_cast<StageWithPredecessor*>(stages[i]);
      stage->readyOut(stageReady[i-1]); stage->dataIn(dataToStage[i-1]);
    }

    // All stages except the last have some ports.
    if(i<stages.size()-1) {
      StageWithSuccessor* stage = dynamic_cast<StageWithSuccessor*>(stages[i]);
      stage->dataOut(dataFromStage[i]);
    }
  }

  // Wire up unique inputs/outputs which can't be done in the loops above.
  FetchStage*  fetch  = dynamic_cast<FetchStage*>(stages.front());
  fetch->toIPKFIFO(dataToBuffers[0]);     fetch->flowControl[0](fcFromBuffers[0]);
  fetch->toIPKCache(dataToBuffers[1]);    fetch->flowControl[1](fcFromBuffers[1]);
  fetch->readyIn(stallRegReady[0]);

  DecodeStage* decode = dynamic_cast<DecodeStage*>(stages[1]);
  decode->fetchOut(fetchSignal);
  decode->flowControlIn(stageReady[stages.size()-2]); // Flow control from output buffers
  decode->readyIn(stallRegReady[1]);
  for(uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    decode->rcetIn[i](dataToBuffers[i+2]);
    decode->flowControlOut[i](fcFromBuffers[i+2]);
  }

  WriteStage*  write  = dynamic_cast<WriteStage*>(stages.back());
  write->fromFetchLogic(fetchSignal);
  write->output(dataOut[0]);              write->creditsIn(creditsIn[0]);
  write->validOutput(validDataOut[0]);    write->validCredit(validCreditIn[0]);
  write->ackOutput(ackDataOut[0]);

  SC_METHOD(updateIdle);
  for(uint i=0; i<stages.size(); i++) sensitive << stageIdle[i];
  // do initialise

  // Initialise the values in some wires.
  idle.initialize(true);
}

Cluster::~Cluster() {
  delete inputCrossbar;

  delete[] stageIdle;
  delete[] stallRegReady;
  delete[] stageReady;
  delete[] dataToStage;
  delete[] dataFromStage;

  for(uint i=0; i<stages.size(); i++) delete stages[i];
  for(uint i=0; i<stallRegs.size(); i++) delete stallRegs[i];
}
