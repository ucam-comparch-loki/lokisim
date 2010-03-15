/*
 * DecodeStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "DecodeStage.h"

/* Direct any new inputs to their destinations every clock cycle. */
void DecodeStage::newCycle() {
  while(true) {
    if(!stall.read()) {
      COPY_IF_NEW(instructionIn, instructionSig);
    }

    for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) {
      COPY_IF_NEW(in[i], fromNetwork[i]);
    }

    wait(clock.posedge_event());
  }
}

/* Received data from the first register output -- send it on to the ALU. */
void DecodeStage::receivedFromRegs1() {
  Data d = static_cast<Data>(regIn1.read());
  regOut1.write(d);
  baseAddress.write(d);
}

/* Received data from the second register output -- send it on to the ALU. */
void DecodeStage::receivedFromRegs2() {
  Data d = static_cast<Data>(regIn2.read());
  regOut2.write(d);
}

/* Update this component's stall status signal. */
void DecodeStage::updateStall() {
  stallOut.write(flStallSig.read() || rcetStallSig.read());
}

DecodeStage::DecodeStage(sc_module_name name, int ID) :
    PipelineStage(name),
    fl("fetchlogic", ID),     // Needs ID so it can generate a return address
    rcet("rcet"),
    decoder("decoder"),
    extend("signextend") {

  in = new sc_in<Word>[NUM_RECEIVE_CHANNELS];
  flowControlOut = new sc_out<bool>[NUM_RECEIVE_CHANNELS];
  fromNetwork = new sc_buffer<Word>[NUM_RECEIVE_CHANNELS];

// Register methods
  SC_METHOD(receivedFromRegs1);
  sensitive << regIn1;
  dont_initialize();

  SC_METHOD(receivedFromRegs2);
  sensitive << regIn2;
  dont_initialize();

  SC_METHOD(updateStall);
  sensitive << flStallSig << rcetStallSig;
  // do initialise

// Connect everything up
  decoder.instructionIn(instructionSig);

  for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    rcet.fromNetwork[i](fromNetwork[i]);
    rcet.flowControl[i](flowControlOut[i]);
  }

  fl.toIPKC(address);
  fl.cacheContainsInst(cacheHit);
  fl.toNetwork(out1);
  fl.flowControl(flowControlIn);
  fl.baseAddress(baseAddress);
  fl.isRoomToFetch(roomToFetch);
  fl.stallOut(flStallSig);

  decoder.regAddr1(regReadAddr1);
  decoder.regAddr2(regReadAddr2);
  decoder.writeAddr(writeAddr);
  decoder.indWrite(indWriteAddr);
  decoder.instructionOut(remoteInst);
  decoder.isIndirectRead(isIndirect);
  decoder.predicate(predicate);
  decoder.setPredicate(setPredicate);
  decoder.toFetchLogic(decodeToFetch); fl.in(decodeToFetch);
  decoder.rChannel(remoteChannel);

  decoder.toRCET1(decodeToRCET1); rcet.fromDecoder1(decodeToRCET1);
  decoder.toRCET2(decodeToRCET2); rcet.fromDecoder2(decodeToRCET2);
  decoder.toSignExtend(decodeToExtend); extend.input(decodeToExtend);

  decoder.operation(operation);
  decoder.op1Select(op1Select);
  decoder.op2Select(op2Select);

  rcet.toALU1(chEnd1);
  rcet.toALU2(chEnd2);
  rcet.stallOut(rcetStallSig);

  extend.output(sExtend);

}

DecodeStage::~DecodeStage() {

}
