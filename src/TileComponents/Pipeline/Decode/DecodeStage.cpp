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

void         DecodeStage::newInput(DecodedInst& inst) {
  if(DEBUG) cout << decoder.name() << " received Instruction: "
                 << dataIn.read() << endl;

  while(true) {
    DecodedInst decoded;
    bool success = decoder.decodeInstruction(inst, decoded);

    if(success) dataOut.write(decoded);

    // If the decoder is ready, we have finished the decode.
    if(decoder.ready()) break;
    // Try again next cycle if the decoder is still busy.
    else wait(clock.posedge_event());
  }
}

void         DecodeStage::sendOutputs() {
  fl.send();
}

bool         DecodeStage::isStalled() const {
  return !decoder.ready();  // Take into account fetch buffers too?
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
    StageWithSuccessor(name),
    StageWithPredecessor(name),
    fl("fetchlogic", ID),     // Needs ID so it can generate a return address
    rcet("rcet"),
    decoder("decoder", ID),
    extend("signextend") {

  id = ID;

  rcetIn         = new sc_in<Word>[NUM_RECEIVE_CHANNELS];
  flowControlOut = new sc_out<int>[NUM_RECEIVE_CHANNELS];

  // Connect everything up
  for(uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    rcet.fromNetwork[i](rcetIn[i]);
    rcet.flowControl[i](flowControlOut[i]);
  }

  fl.toNetwork(fetchOut);
  fl.flowControl(flowControlIn);

}

DecodeStage::~DecodeStage() {
  delete[] rcetIn;
  delete[] flowControlOut;
}
