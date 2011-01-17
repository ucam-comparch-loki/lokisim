/*
 * Cluster.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Cluster.h"
#include "../Utility/InstructionMap.h"

double   Cluster::area() const {
  return regs.area()   + pred.area()    + fetch.area() +
         decode.area() + execute.area() + write.area();
}

double   Cluster::energy() const {
  return regs.energy()   + pred.energy()    + fetch.energy() +
         decode.energy() + execute.energy() + write.energy();
}

/* Initialise the instructions a Cluster will execute. */
void     Cluster::storeData(std::vector<Word>& data, MemoryAddr location) {
  std::vector<Instruction> instructions;

  // Convert all of the words to instructions
  for(uint i=0; i<data.size(); i++) {
    instructions.push_back(static_cast<Instruction>(data[i]));
  }

  fetch.storeCode(instructions);
}

Address Cluster::getInstIndex() const {
  return fetch.getInstIndex();
}

bool     Cluster::inCache(Address a) {
  return fetch.inCache(a);
}

bool     Cluster::roomToFetch() const {
  return fetch.roomToFetch();
}

void     Cluster::refetch() {
  decode.refetch();
}

void     Cluster::jump(JumpOffset offset) {
  fetch.jump(offset);
}

void     Cluster::setPersistent(bool persistent) {
  fetch.setPersistent(persistent);
}

int32_t  Cluster::readReg(RegisterIndex reg, bool indirect) const {

  int result;

  // Check to see if the requested register is one of the previous two
  // registers written to. If so, we forward the data because the register
  // might not have been written to yet.
  // Slightly complicated by the possibility of indirect register reads.
  if(reg>0) {
    if(reg == previousDest1) {
      if(DEBUG) cout << this->name() << " forwarded contents of register "
                     << (int)reg << endl;

      if(indirect) result = regs.read(previousResult1, false);
      else result = previousResult1;
    }
    else if(reg == previousDest2) {
      if(DEBUG) cout << this->name() << " forwarded contents of register "
                     << (int)reg << endl;

      if(indirect) result = regs.read(previousResult2, false);
      else result = previousResult2;
    }
    else result = regs.read(reg, indirect);
  }
  else result = regs.read(reg, indirect);

  return result;
}

int32_t  Cluster::readRegDebug(RegisterIndex reg) const {
  return regs.readDebug(reg);
}

void     Cluster::writeReg(RegisterIndex reg, int32_t value, bool indirect) {
  regs.write(reg, value, indirect);
}

void Cluster::checkForwarding(DecodedInst& inst) const {
  // We don't want to use forwarded data if we read from register 0: this could
  // mean that there is just no register specified, and we are using an
  // immediate instead.
  if(inst.sourceReg1() > 0) {
    if(inst.sourceReg1() == previousDest1) inst.operand1(previousResult1);
    else if(inst.sourceReg1() == previousDest2) inst.operand1(previousResult2);
  }
  if(inst.sourceReg2() > 0) {
    if(inst.sourceReg2() == previousDest1) inst.operand2(previousResult1);
    else if(inst.sourceReg2() == previousDest2) inst.operand2(previousResult2);
  }
}

void Cluster::updateForwarding(const DecodedInst& inst) {
  previousDest2   = previousDest1;
  previousResult2 = previousResult1;

  // We don't want to forward any data which was sent to register 0, because
  // r0 doesn't store values: it is a constant.
  // We also don't want to forward data after an indirect write.
  previousDest1   = (inst.destination() == 0 ||
                     inst.operation() == InstructionMap::IWTR)
                  ? -1 : inst.destination();
  previousResult1 = inst.result();
}

bool     Cluster::readPredReg() const {
  return pred.read();
}

void     Cluster::writePredReg(bool val) {
  pred.write(val);
}

int32_t  Cluster::readRCET(ChannelIndex channel) {
  return decode.readRCET(channel);
}

void     Cluster::updateCurrentPacket(Address addr) {
  regs.updateCurrentIPK(addr);
}

void     Cluster::pipelineStalled(bool stalled) {
//  Instrumentation::stalled(id, stalled);

  currentlyStalled = stalled;

  if(DEBUG) {
    cout << this->name() << ": ";
    if(stalled) cout << "pipeline stalled" << endl;
    else        cout << "pipeline unstalled" << endl;
  }
}

bool     Cluster::discardInstruction(int stage) {
  switch(stage) {
    case 2 : return stallReg1.discard();
    case 3 : return stallReg2.discard();
    case 4 : return stallReg3.discard();
    default : {
      cerr << "Discarding instruction before invalid pipeline stage." << endl;
      throw std::exception();
    }
  }
}

void     Cluster::multiplexOutput0() {

  while(true) {
    // Wait for a signal to arrive.
    wait(out0Decode.default_event() | out0Write.default_event());

    // Whilst dealing with new outputs, more outputs may appear, so we need
    // a loop.
    while(true) {
      // If both signals arrive at the same time, the write signal gets
      // priority. This is because the write will probably be setting up a
      // connection with a memory, and the decode signal will probably be a
      // subsequent fetch from that memory.
      if(out0Write.event()) {
        out[0].write(out0Write.read());
        wait(1, sc_core::SC_NS);
      }
      else if(out0Decode.event()){
        out[0].write(out0Decode.read());
        wait(1, sc_core::SC_NS);
      }
      // If there are no more new inputs, stop looping.
      else break;
    }
  }

}

void     Cluster::updateIdle() {
  constantHigh.write(true);   // Hack: find a better way of doing this.

  bool isIdle = fetchIdle.read()   && decodeIdle.read() &&
                executeIdle.read() && writeIdle.read();

  // Is this what we really want?
  idle.write(isIdle || currentlyStalled);

  Instrumentation::idle(id, isIdle);
}

/* Returns the channel ID of this cluster's instruction packet FIFO. */
ChannelID Cluster::IPKFIFOInput(ComponentID ID) {
  return inputPortID(ID, 0);
}

/* Returns the channel ID of this cluster's instruction packet cache. */
ChannelID Cluster::IPKCacheInput(ComponentID ID) {
  return inputPortID(ID, 1);
}

/* Returns the channel ID of this cluster's specified input channel. */
ChannelID Cluster::RCETInput(ComponentID ID, ChannelIndex channel) {
  return inputPortID(ID, 2 + channel);
}

Cluster::Cluster(sc_module_name name, ComponentID ID) :
    TileComponent(name, ID),
    regs("regs"),
    pred("predicate"),
    stallReg1("stall1"),
    stallReg2("stall2"),
    stallReg3("stall3"),
    write("write", ID),
    execute("execute", ID),
    decode("decode", ID),
    fetch("fetch", ID) {

  flowControlOut = new sc_out<int>[NUM_CORE_INPUTS];
  in             = new sc_in<Word>[NUM_CORE_INPUTS];

  flowControlIn  = new sc_in<bool>[NUM_CORE_OUTPUTS];
  out            = new sc_out<AddressedWord>[NUM_CORE_OUTPUTS];

  previousDest1 = previousDest2 = -1;

  SC_METHOD(updateIdle);
  sensitive << fetchIdle << decodeIdle << executeIdle << writeIdle;
  // do initialise

  SC_THREAD(multiplexOutput0);

// Connect things up
  // Inputs
  fetch.toIPKFIFO(in[0]);
  fetch.toIPKCache(in[1]);
  fetch.flowControl[0](flowControlOut[0]);
  fetch.flowControl[1](flowControlOut[1]);

  for(uint i=2; i<NUM_CORE_INPUTS; i++) {
    decode.rcetIn[i-2](in[i]);
    decode.flowControlOut[i-2](flowControlOut[i]);
  }

  // Outputs
  decode.flowControlIn(flowControlIn[0]);
  write.flowControl[0](flowControlIn[0]);

  decode.fetchOut(out0Decode);
  write.output[0](out0Write);

  // Skip 0 because that is dealt with elsewhere.
  for(uint i=1; i<NUM_CORE_OUTPUTS; i++) {
    write.flowControl[i](flowControlIn[i]);
    write.output[i](out[i]);
  }

  // Clock.
  fetch.clock(clock);                     decode.clock(clock);
  execute.clock(clock);                   write.clock(clock);

  stallReg1.clock(clock);    stallReg2.clock(clock);    stallReg3.clock(clock);

  // Idle signals -- not necessary, but useful for stopping simulation when
  // work is finished.
  fetch.idle(fetchIdle);                  decode.idle(decodeIdle);
  execute.idle(executeIdle);              write.idle(writeIdle);

  // Ready signals -- behave like flow control within the pipeline.
  fetch.readyIn(stall1Ready);             stallReg1.readyOut(stall1Ready);
  stallReg1.readyIn(stall2Ready);         stallReg2.readyOut(stall2Ready);
  stallReg2.readyIn(stall3Ready);         stallReg3.readyOut(stall3Ready);
  stallReg3.readyIn(constantHigh);  // There is no ready input signal

  decode.stallOut(decodeStalled);         stallReg1.localStageStalled(decodeStalled);
  execute.stallOut(executeStalled);       stallReg2.localStageStalled(executeStalled);
  write.stallOut(writeStalled);           stallReg3.localStageStalled(writeStalled);

  // Main data transmission along pipeline.
  fetch.dataOut(fetchToStall1);           stallReg1.dataIn(fetchToStall1);
  decode.dataOut(decodeToStall2);         stallReg2.dataIn(decodeToStall2);
  execute.dataOut(executeToStall3);       stallReg3.dataIn(executeToStall3);

  stallReg1.dataOut(stall1ToDecode);      decode.dataIn(stall1ToDecode);
  stallReg2.dataOut(stall2ToExecute);     execute.dataIn(stall2ToExecute);
  stallReg3.dataOut(stall3ToWrite);       write.dataIn(stall3ToWrite);

  // Initialise the values in some wires.
  idle.initialize(true);

  end_module(); // Needed because we're using a different Component constructor
}

Cluster::~Cluster() {

}
