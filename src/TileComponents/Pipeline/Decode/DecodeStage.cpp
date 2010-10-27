/*
 * DecodeStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "DecodeStage.h"
#include "../../Cluster.h"

double       DecodeStage::area() const {
  return fl.area() + rcet.area() + decoder.area() + extend.area();
}

double       DecodeStage::energy() const {
  return fl.energy() + rcet.energy() + decoder.energy() + extend.energy();
}

void         DecodeStage::updateReady() {
  readyOut.write(true);

  while(true) {
    wait(clock.negedge_event());

    // Send any fetches we may have created in this cycle (or which haven't
    // yet been able to send).
    fl.send();

    // Take into account fetch logic buffer too?
    readyOut.write(readyIn.read() && decoder.ready());
  }
}

void         DecodeStage::execute() {
  idle.write(true);

  // Allow any signals to propagate before starting execution.
  wait(sc_core::SC_ZERO_TIME);

  while(true) {
    // Wait for a new instruction to arrive.
    wait(instructionIn.default_event());

    // Decode the instruction. We are currently not idle.
    idle.write(false);
    decode(instructionIn.read());

    // Once the next cycle starts, revert to being idle.
    wait(clock.posedge_event());
    idle.write(true);
  }
}

void         DecodeStage::decode(const DecodedInst& i) {
  if(DEBUG) cout << decoder.name() << " received Instruction: "
                 << instructionIn.read() << endl;

  while(true) {
    DecodedInst decoded;
    bool success = decoder.decodeInstruction(i, decoded);

    if(success) instructionOut.write(decoded);

    // If the decoder is ready, we have finished the decode.
    if(decoder.ready()) break;
    // Try again next cycle if the decoder is still busy.
    else wait(clock.posedge_event());
  }
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

void         DecodeStage::fetch(uint16_t addr) {
  fl.fetch(addr);
}

void         DecodeStage::setFetchChannel(ChannelID channelID) {
  fl.setFetchChannel(channelID);
}

void         DecodeStage::refetch() {
  fl.refetch();
}

bool         DecodeStage::inCache(Address a) {
  return parent()->inCache(a);
}

bool         DecodeStage::roomToFetch() const {
  return parent()->roomToFetch();
}

void         DecodeStage::jump(int16_t offset) {
  parent()->jump(offset);
}

void         DecodeStage::setPersistent(bool persistent) {
  parent()->setPersistent(persistent);
}

DecodeStage::DecodeStage(sc_module_name name, ComponentID ID) :
    PipelineStage(name),
    fl("fetchlogic", ID),     // Needs ID so it can generate a return address
    rcet("rcet"),
    decoder("decoder", ID),
    extend("signextend") {

  id = ID;

  in             = new sc_in<Word>[NUM_RECEIVE_CHANNELS];
  flowControlOut = new sc_out<int>[NUM_RECEIVE_CHANNELS];

  // Connect everything up
  for(uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    rcet.fromNetwork[i](in[i]);
    rcet.flowControl[i](flowControlOut[i]);
  }

  fl.toNetwork(out1);
  fl.flowControl(flowControlIn);

  SC_THREAD(updateReady);

}

DecodeStage::~DecodeStage() {
  delete[] in;
  delete[] flowControlOut;
}
