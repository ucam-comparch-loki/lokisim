/*
 * DecodeStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "DecodeStage.h"

void DecodeStage::newCycle() {
  while(true) {
    instruction.write(inst.read());

    for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) {
      fromNetwork[i].write(in[i].read());
    }

    wait(clock.posedge_event());
  }
}

void DecodeStage::receivedFromRegs1() {
  Data d = static_cast<Data>(regIn1.read());
  regOut1.write(d);
  baseAddress.write(d);
  std::cout << "Should receive address now." << std::endl;
}

void DecodeStage::receivedFromRegs2() {
  Data d = static_cast<Data>(regIn2.read());
  regOut2.write(d);
}

DecodeStage::DecodeStage(sc_core::sc_module_name name, int ID) :
    PipelineStage(name, ID),
    fl("fetchlogic", ID),
    rcet("rcet", ID),
    decoder("decoder", ID),
    extend("signextend", ID) {

  in = new sc_in<Word>[NUM_RECEIVE_CHANNELS];
  flowControlOut = new sc_out<bool>[NUM_RECEIVE_CHANNELS];
  fromNetwork = new sc_signal<Word>[NUM_RECEIVE_CHANNELS];

// Register methods
  SC_METHOD(receivedFromRegs1);
  sensitive << regIn1;
  dont_initialize();

  SC_METHOD(receivedFromRegs2);
  sensitive << regIn2;
  dont_initialize();

// Connect everything up
  decoder.instructionIn(instruction);

  for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    rcet.fromNetwork[i](fromNetwork[i]);
  }

  fl.toIPKC(address);
  fl.cacheContainsInst(cacheHit);
  fl.toNetwork(out1);
  fl.flowControl(flowControlIn);
  fl.baseAddress(baseAddress);
  fl.isRoomToFetch(roomToFetch);

  decoder.regAddr1(regReadAddr1);
  decoder.regAddr2(regReadAddr2);
  decoder.writeAddr(writeAddr);
  decoder.indWrite(indWriteAddr);
  decoder.instructionOut(remoteInst);
  decoder.isIndirectRead(isIndirect);
  decoder.newRChannel(newRChannel);
  decoder.predicate(predicate);
  decoder.toFetchLogic(decodeToFetch); fl.in(decodeToFetch);
  decoder.rChannel(remoteChannel);

  decoder.toRCET1(decodeToRCET1); rcet.fromDecoder1(decodeToRCET1);
  decoder.toRCET2(decodeToRCET2); rcet.fromDecoder2(decodeToRCET2);
  decoder.toSignExtend(decodeToExtend); extend.input(decodeToExtend);

  decoder.operation(operation);
  decoder.op1Select(op1Select);
  decoder.op2Select(op2Select);

  rcet.clock(clock);
  rcet.toALU1(chEnd1);
  rcet.toALU2(chEnd2);

  for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) {
    rcet.flowControl[i](flowControlOut[i]);
  }

  extend.output(sExtend);

}

DecodeStage::~DecodeStage() {

}
