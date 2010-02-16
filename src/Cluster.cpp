/*
 * Cluster.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Cluster.h"

/* Initialise the instructions a Cluster will execute. */
void Cluster::storeCode(std::vector<Instruction>& instructions) {
  fetch.storeCode(instructions);
}

Cluster::Cluster(sc_core::sc_module_name name, int ID) :
    TileComponent(name, ID),
    regs("regs", ID),
    fetch("fetch", ID),
    decode("decode", ID),
    execute("execute", ID),
    write("write", ID) {

// Connect things up
  // Main inputs/outputs
  decode.flowControl(flowControlIn[0]);
  decode.out1(out[0]);

  for(int i=1; i<NUM_CLUSTER_OUTPUTS; i++) {
    write.flowControl[i-1](flowControlIn[i]);
    write.output[i-1](out[i]);
  }

  // Clock
  fetch.clock(clock);
  decode.clock(clock);
  execute.clock(clock);
  write.clock(clock);

  // To/from fetch stage
  fetch.toIPKQueue(in1);
  fetch.toIPKCache(in2);

  decode.address(FLtoIPKC); fetch.address(FLtoIPKC);
  fetch.instruction(nextInst); decode.inst(nextInst);

  fetch.cacheHit(cacheHitSig); decode.cacheHit(cacheHitSig);
  fetch.roomToFetch(roomToFetch); decode.roomToFetch(roomToFetch);

  // To/from decode stage
  decode.in1(in3);
  decode.in2(in4);

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

  decode.remoteInst(decToExInst); execute.remoteInstIn(decToExInst);
  decode.remoteChannel(decToExRChan); execute.remoteChannelIn(decToExRChan);
  decode.newRChannel(d2eNewRChan); execute.newRChannelIn(d2eNewRChan);
  decode.predicate(predicate); execute.predicate(predicate);

  // To/from execute stage
  execute.output(ALUOutput); execute.fromALU1(ALUOutput);
  execute.fromALU2(ALUOutput);
  write.fromALU(ALUOutput);

  execute.remoteInstOut(exToWriteInst); write.inst(exToWriteInst);
  execute.remoteChannelOut(exToWriteRChan); write.remoteChannel(exToWriteRChan);
  execute.newRChannelOut(e2wNewRChan); write.newRChannel(e2wNewRChan);

  execute.writeOut(writeAddr); write.inRegAddr(writeAddr);
  execute.indWriteOut(indWriteAddr); write.inIndAddr(indWriteAddr);

  // To/from write stage
  write.outRegAddr(writeRegAddr); regs.writeAddr(writeRegAddr);
  write.outIndAddr(indirectWrite); regs.indWriteAddr(indirectWrite);
  write.regData(regWriteData); regs.writeData(regWriteData);

}

Cluster::~Cluster() {

}
