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

/* Direct any new inputs to their destinations every clock cycle. */
void         DecodeStage::newCycle() {
  while(true) {
    // Check for new data (should this happen at the beginning or end of
    // the cycle?
    rcet.checkInputs();

    if(!readyIn.read()) {
      readyOut.write(false);
    }
    else if(!decoder.ready()) {
      // If the decoder is stalling, it is because it is carrying out a
      // multi-cycle operation. It needs to be able to complete this.
      // Send the same instruction again to wake the decoder up.
      decode(instructionIn.read());
    }
    else if(!stall.read() && instructionIn.event()) {
      // Not stalled and have new instruction
      decode(instructionIn.read());
    }
    else {
      idle.write(true);  // What if a fetch is waiting to be sent?
      readyOut.write(true);
    }

    // Send any fetches we may have created in this cycle (or which haven't
    // yet been able to send).
    fl.send();

    for(uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
      COPY_IF_NEW(in[i], fromNetwork[i]);
    }

    wait(clock.posedge_event());
  }
}

void         DecodeStage::decode(Instruction i) {
  bool success = decoder.decodeInstruction(instructionIn.read(), decoded);

  if(success) instructionOut.write(decoded);

  readyOut.write(decoder.ready());
  idle.write(false);
}

int32_t      DecodeStage::readReg(uint8_t index, bool indirect) const {
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

void         DecodeStage::fetch(Address a) {
  fl.fetch(a);
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

void         DecodeStage::jump(int8_t offset) {
  parent()->jump(offset);
}

void         DecodeStage::setPersistent(bool persistent) {
  parent()->setPersistent(persistent);
}

DecodeStage::DecodeStage(sc_module_name name, int ID) :
    PipelineStage(name),
    fl("fetchlogic", ID),     // Needs ID so it can generate a return address
    rcet("rcet"),
    decoder("decoder", ID),
    extend("signextend") {

  in             = new sc_in<Word>[NUM_RECEIVE_CHANNELS];
  flowControlOut = new sc_out<int>[NUM_RECEIVE_CHANNELS];
  fromNetwork    = new sc_buffer<Word>[NUM_RECEIVE_CHANNELS];

// Connect everything up

  for(uint i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    rcet.fromNetwork[i](fromNetwork[i]);
    rcet.flowControl[i](flowControlOut[i]);
  }

  fl.toNetwork(out1);
  fl.flowControl(flowControlIn);

}

DecodeStage::~DecodeStage() {

}
