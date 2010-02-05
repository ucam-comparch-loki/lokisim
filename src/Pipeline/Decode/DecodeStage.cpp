/*
 * DecodeStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "DecodeStage.h"

void DecodeStage::newCycle() {
  instruction.write(inst.read());
  remoteInst.write(inst.read());
  fromNetwork1.write(in1.read());
  fromNetwork2.write(in2.read());
}

void DecodeStage::receivedFromRegs1() {
  Data d = static_cast<Data>(regIn1.read());
  regOut1.write(d);
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

  SC_METHOD(receivedFromRegs1);
  sensitive << regIn1;

  SC_METHOD(receivedFromRegs2);
  sensitive << regIn2;

// Connect everything up
  decoder.instruction(instruction);
  rcet.fromNetwork1(fromNetwork1); rcet.fromNetwork2(fromNetwork2);

  fl.toIPKC(address);

  fl.cacheContainsInst(cacheHit);

  decoder.regAddr1(toRegs1); decoder.regAddr2(toRegs2);
  decoder.indRead(indReadAddr);
  decoder.writeAddr(writeAddr);
  decoder.indWrite(indWriteAddr);

  decoder.isIndirectRead(isIndirectRead);

  decoder.toFetchLogic(decodeToFetch); fl.in(decodeToFetch);

  decoder.toRCET1(decodeToRCET1); rcet.fromDecoder1(decodeToRCET1);
  decoder.toRCET2(decodeToRCET2); rcet.fromDecoder2(decodeToRCET2);

  decoder.operation(operation);
  decoder.op1Select(op1Select); decoder.op2Select(op2Select);

  decoder.toSignExtend(decodeToExtend); extend.input(decodeToExtend);

  fl.toNetwork(out1);

  rcet.toALU1(chEnd1); rcet.toALU2(chEnd2);
  extend.output(sExtend);

}

DecodeStage::~DecodeStage() {

}
