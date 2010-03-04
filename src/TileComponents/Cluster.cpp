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

Cluster::Cluster(sc_module_name name, int ID) :
    TileComponent(name, ID),
    regs("regs"),
    fetch("fetch"),
    decode("decode", ID),   // Needs ID so it can generate a return address
    execute("execute"),
    write("write") {

// Connect things up
  // Main inputs/outputs
  decode.flowControlIn(flowControlIn[0]);
  decode.out1(out[0]);

  for(int i=1; i<NUM_CLUSTER_OUTPUTS; i++) {
    write.flowControl[i-1](flowControlIn[i]);
    write.output[i-1](out[i]);
  }

  fetch.toIPKQueue(in[0]);
  fetch.toIPKCache(in[1]);
  fetch.flowControl[0](flowControlOut[0]);
  fetch.flowControl[1](flowControlOut[1]);

  for(int i=2; i<NUM_CLUSTER_INPUTS; i++) {
    decode.in[i-2](in[i]);
    decode.flowControlOut[i-2](flowControlOut[i]);
  }

  // Clock
  fetch.clock(clock);
  decode.clock(clock);
  execute.clock(clock);
  write.clock(clock);

  // To/from fetch stage
  decode.address(FLtoIPKC); fetch.address(FLtoIPKC);
  fetch.instruction(nextInst); decode.instructionIn(nextInst);

  fetch.cacheHit(cacheHitSig); decode.cacheHit(cacheHitSig);
  fetch.roomToFetch(roomToFetch); decode.roomToFetch(roomToFetch);

  // To/from decode stage
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
  decode.predicate(predicate); execute.predicate(predicate);
  decode.setPredicate(setPredSig); execute.setPredicate(setPredSig);

  // To/from execute stage
  execute.output(ALUOutput); execute.fromALU1(ALUOutput);
  execute.fromALU2(ALUOutput);
  write.fromALU(ALUOutput);

  execute.remoteInstOut(exToWriteInst); write.inst(exToWriteInst);
  execute.remoteChannelOut(exToWriteRChan); write.remoteChannel(exToWriteRChan);

  execute.writeOut(writeAddr); write.inRegAddr(writeAddr);
  execute.indWriteOut(indWriteAddr); write.inIndAddr(indWriteAddr);

  // To/from write stage
  write.outRegAddr(writeRegAddr); regs.writeAddr(writeRegAddr);
  write.outIndAddr(indirectWrite); regs.indWriteAddr(indirectWrite);
  write.regData(regWriteData); regs.writeData(regWriteData);

  end_module(); // Needed because we're using a different Component constructor
}

Cluster::~Cluster() {

}
