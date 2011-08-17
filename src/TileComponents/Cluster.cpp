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
  for(unsigned int i=0; i<data.size(); i++) {
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
void     Cluster::refetch(const MemoryAddr addr) {decode.fetch(addr, true);}
void     Cluster::jump(const JumpOffset offset)  {fetch.jump(offset);}
void     Cluster::setPersistent(bool persistent) {fetch.setPersistent(persistent);}

const int32_t Cluster::readReg(RegisterIndex reg, bool indirect) {

  int32_t result;

  // Check to see if the requested register is one which will be written to by
  // an instruction which is still in the pipeline. If so, we forward the data.
  // Slightly complicated by the possibility of indirect register reads.
  if(reg>0) {
    if(reg == execute.currentInstruction().destination()) {
      // If the instruction in the execute stage will write to the register we
      // want to read, but it hasn't produced a result yet, wait until
      // execution completes.
      if(!execute.currentInstruction().hasResult())
        wait(execute.executedEvent());

      result = execute.currentInstruction().result();

      if(DEBUG) cout << this->name() << " forwarded contents of register "
                     << (int)reg << " (" << result << ")" << endl;
      Instrumentation::dataForwarded(id, reg);

      if(indirect) result = regs.read(result, false);
    }
    else if(reg == write.currentInstruction().destination()) {
      // No waiting required this time - if the instruction is in the write
      // stage, it definitely has a result.
      result = write.currentInstruction().result();

      if(DEBUG) cout << this->name() << " forwarded contents of register "
                     << (int)reg << " (" << result << ")" << endl;
      Instrumentation::dataForwarded(id, reg);

      if(indirect) result = regs.read(result, false);
    }
    else result = regs.read(reg, indirect);
  }
  else result = 0;

  return result;
}

/* Read a register without data forwarding and without indirection. */
const int32_t Cluster::readRegDebug(RegisterIndex reg) const {
  return regs.readDebug(reg);
}

void     Cluster::writeReg(RegisterIndex reg, int32_t value, bool indirect) {
  regs.write(reg, value, indirect);
}

bool     Cluster::readPredReg(bool waitForExecution) {
  // The wait parameter tells us to wait for the predicate to be written if
  // the instruction in the execute stage will set it.
  if(waitForExecution && execute.currentInstruction().setsPredicate()
                      && !execute.currentInstruction().hasResult()) {
    wait(execute.executedEvent());
  }

  return pred.read();
}

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
  stallEvent.notify();

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

  // Is this what we really want (including stall state in an idle signal)?
  if((isIdle || currentlyStalled) != idle.read())
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

  inputCrossbar = new InputCrossbar("input_crossbar", ID);

  // Create signals, with the number based on the length of the pipeline.
  stageIdle     = new sc_signal<bool>[4];
  stallRegReady = new sc_signal<bool>[3];
  stageReady    = new sc_signal<bool>[3];
  instToStage   = new flag_signal<DecodedInst>[3];
  instFromStage = new sc_buffer<DecodedInst>[3];

  fcFromBuffers = new sc_signal<bool>[CORE_INPUT_CHANNELS];
  dataToBuffers = new sc_buffer<Word>[CORE_INPUT_CHANNELS];

  // Wire the input ports to the input buffers.
  for(unsigned int i=0; i<CORE_INPUT_PORTS; i++) {
    inputCrossbar->dataIn[i](dataIn[i]);
    inputCrossbar->validDataIn[i](validDataIn[i]);
    inputCrossbar->ackDataIn[i](ackDataIn[i]);
  }

  inputCrossbar->creditsOut[0](creditsOut[0]);
  inputCrossbar->validCreditOut[0](validCreditOut[0]);
  inputCrossbar->ackCreditOut[0](ackCreditOut[0]);

  for(unsigned int i=0; i<CORE_INPUT_CHANNELS; i++) {
    inputCrossbar->dataOut[i](dataToBuffers[i]);
    inputCrossbar->bufferHasSpace[i](fcFromBuffers[i]);
  }

  inputCrossbar->clock(clock);
  inputCrossbar->creditClock(fastClock);
  inputCrossbar->dataClock(slowClock);

  // Wire the stall registers up.
  for(unsigned int i=0; i<3; i++) {
    StallRegister* stallReg = new StallRegister(sc_core::sc_gen_unique_name("stall"), i);

    stallReg->clock(clock);               stallReg->readyOut(stallRegReady[i]);
    stallReg->dataIn(instFromStage[i]);   stallReg->dataOut(instToStage[i]);
    stallReg->localStageReady(stageReady[i]);

    // Allow instant propagation from the final stall register to all previous
    // ones. This is required to stop a decoder-only instruction accidentally
    // executing when the pipeline is stalled.
    if(i < 2) stallReg->readyIn(stallRegReady[2]);
    else 			stallReg->readyIn(constantHigh);

    stallRegs.push_back(stallReg);
  }

  // Wire the pipeline stages up.

  fetch.clock(clock);                     fetch.idle(stageIdle[0]);
  fetch.toIPKFIFO(dataToBuffers[0]);      fetch.flowControl[0](fcFromBuffers[0]);
  fetch.toIPKCache(dataToBuffers[1]);     fetch.flowControl[1](fcFromBuffers[1]);
  fetch.readyIn(stallRegReady[0]);				fetch.dataOut(instFromStage[0]);

  decode.clock(clock);                    decode.idle(stageIdle[1]);
  decode.fetchOut(fetchSignal);
  decode.flowControlIn(stageReady[2]); // Flow control from output buffers
  decode.readyIn(stallRegReady[1]);       decode.dataOut(instFromStage[1]);
  decode.readyOut(stageReady[0]);         decode.dataIn(instToStage[0]);
  for(uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    decode.rcetIn[i](dataToBuffers[i+2]);
    decode.flowControlOut[i](fcFromBuffers[i+2]);
  }

  execute.clock(clock);                   execute.idle(stageIdle[2]);
  execute.readyOut(stageReady[1]);        execute.dataIn(instToStage[1]);
  execute.dataOut(instFromStage[2]);

  write.clock(clock);                     write.idle(stageIdle[3]);
  write.readyOut(stageReady[2]);          write.dataIn(instToStage[2]);
  write.fromFetchLogic(fetchSignal);

  for(unsigned int i=0; i<CORE_OUTPUT_PORTS; i++) {
    write.output[i](dataOut[i]);            write.creditsIn[i](creditsIn[i]);
    write.validOutput[i](validDataOut[i]);  write.validCredit[i](validCreditIn[i]);
    write.ackOutput[i](ackDataOut[i]);
  }

  SC_METHOD(updateIdle);
  for(unsigned int i=0; i<4; i++) sensitive << stageIdle[i];
  sensitive << stallEvent;
  // do initialise

  // Initialise the values in some wires.
  idle.initialize(true);
  constantHigh.write(true);
}

Cluster::~Cluster() {
  delete inputCrossbar;

  delete[] stageIdle;  		delete[] stallRegReady;  delete[] stageReady;
  delete[] dataToBuffers; delete[] fcFromBuffers;
  delete[] instToStage;  	delete[] instFromStage;

  for(unsigned int i=0; i<stallRegs.size(); i++) delete stallRegs[i];
}
