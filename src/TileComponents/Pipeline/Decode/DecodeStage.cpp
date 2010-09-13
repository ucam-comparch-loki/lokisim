/*
 * DecodeStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "DecodeStage.h"
#include "../../Cluster.h"

double DecodeStage::area() const {
  return fl.area() + rcet.area() + decoder.area() + extend.area();
}

double DecodeStage::energy() const {
  return fl.energy() + rcet.energy() + decoder.energy() + extend.energy();
}

/* Direct any new inputs to their destinations every clock cycle. */
void DecodeStage::newCycle() {
  while(true) {
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

    for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) {
      COPY_IF_NEW(in[i], fromNetwork[i]);
    }

    wait(clock.posedge_event());
  }
}

void DecodeStage::decode(Instruction i) {
  bool success = decoder.decodeInstruction(instructionIn.read(), decoded);

  if(success) instructionOut.write(decoded);

  readyOut.write(decoder.ready());
  idle.write(false);
}

int32_t DecodeStage::readRegs(uint8_t index, bool indirect) {
  return ((Cluster*)(this->get_parent()))->getRegVal(index, indirect);
}

int32_t DecodeStage::readRCET(uint8_t index) {
  return rcet.read(index);
}

void DecodeStage::fetch(Address a) {

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
  stallOut.write(flStall.read() || rcetStall.read());
}

/* Allow multiple components to request a channel-end read. */
void DecodeStage::updateToRCET1() {
  if(indirectChannel.event()) RCETread1.write(indirectChannel.read());
  else RCETread1.write(decodeToRCET1.read());
}

/* Allow multiple components to select where the ALU's first operand comes
 * from. */
void DecodeStage::updateOp1Select() {
  if(indirectChannel.event()) {
    op1Select.write(Decoder::RCET);  // Indirect regs say to read from RCET
  }
  else if(op1SelectSig.event()) {
    op1Select.write(op1SelectSig.read());
  }
}

DecodeStage::DecodeStage(sc_module_name name, int ID) :
    PipelineStage(name),
    fl("fetchlogic", ID),     // Needs ID so it can generate a return address
    rcet("rcet"),
    decoder("decoder", ID),
    extend("signextend") {

  in = new sc_in<Word>[NUM_RECEIVE_CHANNELS];
  flowControlOut = new sc_out<int>[NUM_RECEIVE_CHANNELS];
  fromNetwork = new sc_buffer<Word>[NUM_RECEIVE_CHANNELS];

// Register methods
  SC_METHOD(receivedFromRegs1);
  sensitive << regIn1;
  dont_initialize();

  SC_METHOD(receivedFromRegs2);
  sensitive << regIn2;
  dont_initialize();

  SC_METHOD(updateToRCET1);
  sensitive << indirectChannel << decodeToRCET1;
  dont_initialize();

  SC_METHOD(updateOp1Select);
  sensitive << indirectChannel << op1SelectSig;

  SC_METHOD(updateStall);
  sensitive << flStall << rcetStall;// << decoderStall;
  // do initialise

// Connect everything up
//  decoder.instructionIn(instructionSig);

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
  fl.refetch(refetch);
  fl.stallOut(flStall);

//  decoder.regAddr1(regReadAddr1);
//  decoder.regAddr2(regReadAddr2);
//  decoder.writeAddr(writeAddr);
//  decoder.indWrite(indWriteAddr);
//  decoder.instructionOut(remoteInst);
//  decoder.isIndirectRead(isIndirect);
//  decoder.jumpOffset(jumpOffset);
//  decoder.persistent(persistent);
//  decoder.memoryOp(memoryOp);
//  decoder.predicate(predicate);
//  decoder.setPredicate(setPredicate);
//  decoder.usePredicate(usePredicate);
//  decoder.toFetchLogic(decodeToFetch);    fl.in(decodeToFetch);
//  decoder.rChannel(remoteChannel);
//  decoder.waitOnChannel(waitOnChannel);
//  decoder.toRCET1(decodeToRCET1);         rcet.fromDecoder1(RCETread1);
//  decoder.toRCET2(decodeToRCET2);         rcet.fromDecoder2(decodeToRCET2);
//  decoder.channelOp(RCETOperation);       rcet.operation(RCETOperation);
//  decoder.toSignExtend(decodeToExtend);   extend.input(decodeToExtend);
//  decoder.stall(stallFetch);
//
//  decoder.operation(operation);
//  decoder.op1Select(op1SelectSig);
//  decoder.op2Select(op2Select);

                                          fl.in(decodeToFetch);
                                          rcet.fromDecoder1(RCETread1);
                                          rcet.fromDecoder2(decodeToRCET2);
                                          rcet.operation(RCETOperation);
                                          extend.input(decodeToExtend);

  rcet.toALU1(chEnd1);
  rcet.toALU2(chEnd2);
  rcet.stallOut(rcetStall);

  extend.output(sExtend);

}

DecodeStage::~DecodeStage() {

}
