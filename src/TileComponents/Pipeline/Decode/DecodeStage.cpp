/*
 * DecodeStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "DecodeStage.h"
#include "../../Cluster.h"
#include "../../../Datatype/DecodedInst.h"

void         DecodeStage::execute() {
  while(true) {
    // Wait for a new instruction to arrive.
    wait(dataIn.default_event());

    // Deal with the new input. We are currently not idle.
    idle.write(false);
    DecodedInst inst = dataIn.read(); // Don't want a const input.
    newInput(inst);

    // Once the next cycle starts, revert to being idle.
    wait(clock.posedge_event());
    idle.write(true);
  }
}

void         DecodeStage::updateReady() {
  // Write our current stall status.
  readyOut.write(!isStalled());

  if(DEBUG && isStalled() && readyOut.read()) {
    cout << this->name() << " stalled." << endl;
  }

  // Wait until some point late in the cycle, so we know that any operations
  // will have completed.
  next_trigger(clock.negedge_event());
}

void         DecodeStage::newInput(DecodedInst& inst) {
  if(DEBUG) cout << decoder.name() << " received Instruction: "
                 << dataIn.read() << endl;

  while(true) {
    DecodedInst decoded;
    bool success = decoder.decodeInstruction(inst, decoded);

    if(success) {
      // Wait until flow control allows us to send.
      if(!readyIn.read()) {
        waitingToSend = true;
        wait(readyIn.posedge_event());
        waitingToSend = false;
      }
      dataOut.write(decoded);
    }

    // If the decoder is ready, we have finished the decode.
    if(decoder.ready()) break;
    // Try again next cycle if the decoder is still busy.
    else wait(clock.posedge_event());
  }
}

bool         DecodeStage::isStalled() const {
  return !decoder.ready() || waitingToSend;  // Take into account fetch buffers too?
}

int32_t      DecodeStage::readReg(RegisterIndex index, bool indirect) const {
  return parent()->readReg(index, indirect);
}

bool         DecodeStage::predicate() const {
  return parent()->readPredReg();
}

int32_t      DecodeStage::readRCET(ChannelIndex index) {
  return rcet.read(index);
}

bool         DecodeStage::testChannel(ChannelIndex index) const {
  return rcet.testChannelEnd(index);
}

ChannelIndex DecodeStage::selectChannel() {
  return rcet.selectChannelEnd();
}

void         DecodeStage::fetch(const MemoryAddr addr) {
  fl.fetch(addr);
}

void         DecodeStage::setFetchChannel(const ChannelID& channelID, uint memoryGroupBits, uint memoryLineBits) {
  fl.setFetchChannel(channelID, memoryGroupBits, memoryLineBits);
}

void         DecodeStage::refetch() {
  fl.refetch();
}

bool         DecodeStage::inCache(const MemoryAddr a) const {
  return parent()->inCache(a);
}

bool         DecodeStage::roomToFetch() const {
  return parent()->roomToFetch();
}

void         DecodeStage::jump(JumpOffset offset) const {
  parent()->jump(offset);
}

void         DecodeStage::setPersistent(bool persistent) const {
  parent()->setPersistent(persistent);
}

bool         DecodeStage::discardNextInst() const {
  // This is the 2nd pipeline stage, so the argument is 2.
  return parent()->discardInstruction(2);
}

DecodeStage::DecodeStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    fl("fetchlogic", ID),
    rcet("rcet", ID),
    decoder("decoder", ID),
    extend("signextend") {
  waitingToSend = false;

  rcetIn         = new sc_in<Word>[NUM_RECEIVE_CHANNELS];
  flowControlOut = new sc_out<bool>[NUM_RECEIVE_CHANNELS];

  // Connect everything up
  rcet.clock(clock);
  for(uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    rcet.fromNetwork[i](rcetIn[i]);
    rcet.flowControl[i](flowControlOut[i]);
  }

  fl.clock(clock);
  fl.toNetwork(fetchOut);
  fl.flowControl(flowControlIn);

  readyOut.initialize(false);

  SC_THREAD(execute);

}

DecodeStage::~DecodeStage() {
  delete[] rcetIn;
  delete[] flowControlOut;
}
