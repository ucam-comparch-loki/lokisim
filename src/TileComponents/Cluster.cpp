/*
 * Cluster.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Cluster.h"

double   Cluster::area() const {
  return regs.area()   + pred.area()    + fetch.area() +
         decode.area() + execute.area() + write.area();
}

double   Cluster::energy() const {
  return regs.energy()   + pred.energy()    + fetch.energy() +
         decode.energy() + execute.energy() + write.energy();
}

/* Initialise the instructions a Cluster will execute. */
void     Cluster::storeData(std::vector<Word>& data, int location) {
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

void     Cluster::jump(int16_t offset) {
  fetch.jump(offset);
}

void     Cluster::setPersistent(bool persistent) {
  fetch.setPersistent(persistent);
}

int32_t  Cluster::readReg(RegisterIndex reg, bool indirect) const {
  return regs.read(reg, indirect);
}

int32_t  Cluster::readRegDebug(RegisterIndex reg) const {
  return regs.readDebug(reg);
}

void     Cluster::writeReg(RegisterIndex reg, int32_t value, bool indirect) {
  regs.write(reg, value, indirect);
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
  Instrumentation::stalled(id, stalled);

  currentlyStalled = stalled;

  if(DEBUG) {
    cout << this->name() << ": ";
    if(stalled) cout << "pipeline stalled" << endl;
    else        cout << "pipeline unstalled" << endl;
  }
}

void     Cluster::multiplexOutput0() {
  // Note: we absolutely must not send two outputs to this port on the same
  // cycle.
  if(out0Decode.event()) out[0].write(out0Decode.read());
  else if(out0Write.event()) out[0].write(out0Write.read());
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
  return ID*NUM_CLUSTER_INPUTS + 0;
}

/* Returns the channel ID of this cluster's instruction packet cache. */
ChannelID Cluster::IPKCacheInput(ComponentID ID) {
  return ID*NUM_CLUSTER_INPUTS + 1;
}

/* Returns the channel ID of this cluster's specified input channel. */
ChannelID Cluster::RCETInput(ComponentID ID, ChannelIndex channel) {
  return ID*NUM_CLUSTER_INPUTS + 2 + channel;
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

  SC_METHOD(updateIdle);
  sensitive << fetchIdle << decodeIdle << executeIdle << writeIdle;
  // do initialise

  SC_METHOD(multiplexOutput0);
  sensitive << decode.fetchOut << write.output[0];
  dont_initialize();

// Connect things up
  // Inputs
  fetch.toIPKFIFO(in[0]);
  fetch.toIPKCache(in[1]);
  fetch.flowControl[0](flowControlOut[0]);
  fetch.flowControl[1](flowControlOut[1]);

  for(uint i=2; i<NUM_CLUSTER_INPUTS; i++) {
    decode.rcetIn[i-2](in[i]);
    decode.flowControlOut[i-2](flowControlOut[i]);
  }

  // Outputs
  decode.flowControlIn(flowControlIn[0]);
  write.flowControl[0](flowControlIn[0]);

  decode.fetchOut(out0Decode);
  write.output[0](out0Write);

  // Skip 0 because that is dealt with elsewhere.
  for(uint i=1; i<NUM_CLUSTER_OUTPUTS; i++) {
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
  fetch.dataOut(fetchToDecode);           stallReg1.dataIn(fetchToDecode);
  decode.dataOut(decodeToExecute);        stallReg2.dataIn(decodeToExecute);
  execute.dataOut(executeToWrite);        stallReg3.dataIn(executeToWrite);

  stallReg1.dataOut(fetchToDecode2);      decode.dataIn(fetchToDecode2);
  stallReg2.dataOut(decodeToExecute2);    execute.dataIn(decodeToExecute2);
  stallReg3.dataOut(executeToWrite2);     write.dataIn(executeToWrite2);

  end_module(); // Needed because we're using a different Component constructor
}

Cluster::~Cluster() {

}
