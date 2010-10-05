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
void     Cluster::storeData(std::vector<Word>& data) {
  std::vector<Instruction> instructions;

  // Convert all of the words to instructions
  for(uint i=0; i<data.size(); i++) {
    instructions.push_back(static_cast<Instruction>(data[i]));
  }

  fetch.storeCode(instructions);
}

uint16_t Cluster::getInstIndex() const {
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

void     Cluster::pipelineStalled(bool stalled) {
  // TODO: collect timestamps in instrumentation.
  Instrumentation::stalled(id, stalled,
      sc_core::sc_time_stamp().to_default_time_units());

  currentlyStalled = stalled;

  if(DEBUG) {
    cout << this->name() << ": ";
    if(stalled) cout << "pipeline stalled" << endl;
    else        cout << "pipeline unstalled" << endl;
  }
}

void     Cluster::updateIdle() {
  bool isIdle = fetchIdle.read()   && decodeIdle.read() &&
                executeIdle.read() && writeIdle.read();

  // Is this what we really want?
  idle.write(isIdle || currentlyStalled);

  Instrumentation::idle(id, isIdle,
      sc_core::sc_time_stamp().to_default_time_units());
}

void     Cluster::updateCurrentPacket(Address addr) {
  regs.updateCurrentIPK(addr);
}

/* Returns the channel ID of this cluster's instruction packet FIFO. */
uint32_t Cluster::IPKFIFOInput(uint16_t ID) {
  return ID*NUM_CLUSTER_INPUTS + 0;
}

/* Returns the channel ID of this cluster's instruction packet cache. */
uint32_t Cluster::IPKCacheInput(uint16_t ID) {
  return ID*NUM_CLUSTER_INPUTS + 1;
}

/* Returns the channel ID of this cluster's specified input channel. */
uint32_t Cluster::RCETInput(uint16_t ID, ChannelIndex channel) {
  return ID*NUM_CLUSTER_INPUTS + 2 + channel;
}

Cluster::Cluster(sc_module_name name, uint16_t ID) :
    TileComponent(name, ID),
    regs("regs"),
    pred("predicate"),
    write("write"),
    execute("execute", ID),
    decode("decode", ID),   // Needs ID so it can generate a return address
    fetch("fetch") {

  SC_METHOD(updateIdle);
  sensitive << fetchIdle << decodeIdle << executeIdle << writeIdle;
  // do initialise

// Connect things up
  // Main inputs/outputs
  decode.flowControlIn(flowControlIn[0]);
  decode.out1(out[0]);

  for(uint i=1; i<NUM_CLUSTER_OUTPUTS; i++) {
    write.flowControl[i-1](flowControlIn[i]);
    write.output[i-1](out[i]);
  }

  fetch.toIPKFIFO(in[0]);
  fetch.toIPKCache(in[1]);
  fetch.flowControl[0](flowControlOut[0]);
  fetch.flowControl[1](flowControlOut[1]);

  for(uint i=2; i<NUM_CLUSTER_INPUTS; i++) {
    decode.in[i-2](in[i]);
    decode.flowControlOut[i-2](flowControlOut[i]);
  }

  // Clock.
  fetch.clock(clock);                     decode.clock(clock);
  execute.clock(clock);                   write.clock(clock);

  // Idle signals -- not necessary, but useful for stopping simulation when
  // work is finished.
  fetch.idle(fetchIdle);                  decode.idle(decodeIdle);
  execute.idle(executeIdle);              write.idle(writeIdle);

  // Ready signals -- behave like flow control within the pipeline.
  fetch.readyIn(decodeReady);             decode.readyOut(decodeReady);
  decode.readyIn(executeReady);           execute.readyOut(executeReady);
  execute.readyIn(writeReady);            write.readyOut(writeReady);

  // Main data transmission along pipeline.
  fetch.instruction(fetchToDecode);       decode.instructionIn(fetchToDecode);
  decode.instructionOut(decodeToExecute); execute.operation(decodeToExecute);
  execute.result(executeToWrite);         write.result(executeToWrite);

  end_module(); // Needed because we're using a different Component constructor
}

Cluster::~Cluster() {

}
