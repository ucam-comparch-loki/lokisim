/*
 * Cluster.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Cluster.h"

/* Combine all of the outputs into one vector */
void Cluster::combineOutputs() {
  // Put the decoder's output into an Array
  Array<AddressedWord> *decodeOut = new Array<AddressedWord>(1);
  AddressedWord aw = decode.out1.read();
  decodeOut->put(0, aw);

  // Merge the array with that from the write stage
  Array<AddressedWord> writeOut = write.output.read();   // Try to remove copy
  decodeOut->merge(writeOut);

  out.write(*decodeOut);
}

Cluster::Cluster(sc_core::sc_module_name name, int ID) :
    TileComponent(name, ID),
    regs("regs", ID),
    fetch("fetch", ID),
    decode("decode", ID),
    execute("execute", ID),
    write("write", ID) {

  SC_METHOD(combineOutputs);
  sensitive << write.output << decode.out1;

// Connect things up
  // Clock
//  clock(fetchClock); fetch.clock(fetchClock);
//  clock(decodeClock); decode.clock(decodeClock);
//  clock(executeClock); execute.clock(executeClock);
//  clock(writeClock); write.clock(writeClock);
  fetch.clock(clock);
  decode.clock(clock);
  execute.clock(clock);
  write.clock(clock);

  // To/from fetch stage
//  in1(in1toIPKQ); fetch.toIPKQueue(in1toIPKQ);
//  in2(in2toIPKC); fetch.toIPKCache(in2toIPKC);
  fetch.toIPKQueue(in1);
  fetch.toIPKCache(in2);

  decode.address(FLtoIPKC); fetch.address(FLtoIPKC);

  fetch.instruction(nextInst); decode.inst(nextInst);
  fetch.instruction(sendInst); write.inst(sendInst);

  fetch.cacheHit(cacheHitSig); decode.cacheHit(cacheHitSig);

  // To/from decode stage
//  in3(in3toRCET); decode.in1(in3toRCET);
//  in4(in4toRCET); decode.in2(in4toRCET);
  decode.in1(in3);
  decode.in2(in4);

//  sc_signal<AddressedWord> fetching;
//  out1(fetching); decode.out1(fetching);

  decode.regIn1(regData1); regs.out1(regData1);
  decode.regIn2(regData2); regs.out2(regData2);

  decode.regReadAddr1(regRead1); regs.readAddr1(regRead1);
  decode.regReadAddr2(regRead2); regs.readAddr2(regRead2);
  decode.writeAddr(decWriteAddr); execute.writeIn(decWriteAddr);
  decode.indWriteAddr(decIndWrite); execute.indWriteIn(decIndWrite);

  decode.isIndirect(indirectReadSig); regs.indRead(indirectReadSig);

  decode.chEnd1(RCETtoALU1); execute.fromRChan1(RCETtoALU1);
  decode.chEnd2(RCETtoALU2); execute.fromRChan2(RCETtoALU2);
  decode.regOut1(regToALU1); execute.fromReg1(regToALU1);
  decode.regOut2(regToALU2); execute.fromReg2(regToALU2);
  decode.sExtend(SEtoALU); execute.fromSExtend(SEtoALU);

  decode.operation(operation); execute.operation(operation);
  decode.op1Select(op1Select); execute.op1Select(op1Select);
  decode.op2Select(op2Select); execute.op2Select(op2Select);

  decode.remoteInst(decodeToExecute); execute.remoteInstIn(decodeToExecute);

  // To/from execute stage
  execute.remoteInstOut(executeToWrite); write.inst(executeToWrite);

  execute.output(ALUtoALU1); execute.fromALU1(ALUtoALU1);
  execute.output(ALUtoALU2); execute.fromALU2(ALUtoALU2);
  execute.output(ALUtoWrite); write.fromALU(ALUtoWrite);

  execute.writeOut(writeAddr); write.inRegAddr(writeAddr);
  execute.indWriteOut(indWriteAddr); write.inIndAddr(indWriteAddr);

  // To/from write stage
//  sc_signal<AddressedWord> SCETtoOut2;
//  write.output(SCETtoOut2); out2(SCETtoOut2);

  write.outRegAddr(writeRegAddr); regs.writeAddr(writeRegAddr);
  write.outIndAddr(indirectWrite); regs.indWriteAddr(indirectWrite);

  write.regData(regWriteData); regs.writeData(regWriteData);

}

Cluster::~Cluster() {

}
