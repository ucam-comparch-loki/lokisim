/*
 * Cluster.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Cluster.h"

double Cluster::area() const {
  return regs.area()   + pred.area()    + fetch.area() +
         decode.area() + execute.area() + write.area();
}

double Cluster::energy() const {
  return regs.energy()   + pred.energy()    + fetch.energy() +
         decode.energy() + execute.energy() + write.energy();
}

/* Initialise the instructions a Cluster will execute. */
void Cluster::storeData(std::vector<Word>& data) {
  std::vector<Instruction> instructions;

  // Convert all of the words to instructions
  for(unsigned int i=0; i<data.size(); i++) {
    instructions.push_back(static_cast<Instruction>(data[i]));
  }

  fetch.storeCode(instructions);
}

/* Checks the status signals of various pipeline stages to determine if the
 * pipeline should be stalled/unstalled. */
void Cluster::stallPipeline() {
  bool shouldStall = decStallSig.read() || writeStallSig.read();
  stallSig.write(shouldStall);

  Instrumentation::stalled(id, shouldStall,
      sc_core::sc_time_stamp().to_default_time_units());

  // The fetch stage has to be stalled more often than the rest of the
  // pipeline, due to multi-cycle instructions which require that no further
  // instructions are decoded until they complete.
  fetchStallSig.write(shouldStall || stallFetchSig.read());

  if(DEBUG) {
    cout << this->name() << ": ";
    if(shouldStall) cout << "stalling pipeline" << endl;
    else            cout << "unstalling pipeline" << endl;
  }
}

void Cluster::updateCurrentPacket() {
  regs.updateCurrentIPK(currentIPKSig.read());
}

/* Returns the channel ID of this cluster's instruction packet FIFO. */
int Cluster::IPKFIFOInput(int ID) {
  return ID*NUM_CLUSTER_INPUTS + 0;
}

/* Returns the channel ID of this cluster's instruction packet cache. */
int Cluster::IPKCacheInput(int ID) {
  return ID*NUM_CLUSTER_INPUTS + 1;
}

/* Returns the channel ID of this cluster's specified input channel. */
int Cluster::RCETInput(int ID, int channel) {
  return ID*NUM_CLUSTER_INPUTS + 2 + channel;
}

Cluster::Cluster(sc_module_name name, int ID) :
    TileComponent(name, ID),
    regs("regs"),
    pred("pred"),
    fetch("fetch"),
    decode("decode", ID),   // Needs ID so it can generate a return address
    execute("execute"),
    write("write") {

  SC_METHOD(stallPipeline);
  sensitive << decStallSig << writeStallSig;
  // do initialise

  SC_METHOD(updateCurrentPacket);
  sensitive << currentIPKSig;
  dont_initialize();

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

  // Clock and stall signal
  fetch.clock(clock);                 fetch.stall(fetchStallSig);
  decode.clock(clock);                decode.stall(stallSig);
                                      decode.stallFetch(stallFetchSig);
  execute.clock(clock);               execute.stall(stallSig);
  write.clock(clock);                 write.stall(stallSig);

  // To/from fetch stage
  decode.address(FLtoIPKC);           fetch.address(FLtoIPKC);
  decode.jumpOffset(jumpOffsetSig);   fetch.jumpOffset(jumpOffsetSig);
  decode.persistent(persistent);      fetch.persistent(persistent);
  fetch.instruction(nextInst);        decode.instructionIn(nextInst);

  fetch.cacheHit(cacheHitSig);        decode.cacheHit(cacheHitSig);
  fetch.roomToFetch(roomToFetch);     decode.roomToFetch(roomToFetch);
  fetch.refetch(refetchSig);          decode.refetch(refetchSig);
  fetch.currentPacket(currentIPKSig);

  // To/from decode stage
  decode.regIn1(regData1);            regs.out1(regData1);
  decode.regIn2(regData2);            regs.out2(regData2);

  decode.regReadAddr1(regRead1);      regs.readAddr1(regRead1);
  decode.regReadAddr2(regRead2);      regs.readAddr2(regRead2);
  decode.writeAddr(decWriteAddr);     execute.writeIn(decWriteAddr);
  decode.indWriteAddr(decIndWrite);   execute.indWriteIn(decIndWrite);

  decode.isIndirect(indirectReadSig); regs.indRead(indirectReadSig);
  decode.indirectChannel(indChannelSig);  regs.channelID(indChannelSig);

  decode.chEnd1(RCETtoALU1);          execute.fromRChan1(RCETtoALU1);
  decode.chEnd2(RCETtoALU2);          execute.fromRChan2(RCETtoALU2);
  decode.regOut1(regToALU1);          execute.fromReg1(regToALU1);
  decode.regOut2(regToALU2);          execute.fromReg2(regToALU2);
  decode.sExtend(SEtoALU);            execute.fromSExtend(SEtoALU);

  decode.operation(operation);        execute.operation(operation);
  decode.op1Select(op1Select);        execute.op1Select(op1Select);
  decode.op2Select(op2Select);        execute.op2Select(op2Select);

  decode.remoteInst(decToExInst);     execute.remoteInstIn(decToExInst);
  decode.remoteChannel(decToExRChan); execute.remoteChannelIn(decToExRChan);
  decode.memoryOp(decToExMemOp);      execute.memoryOpIn(decToExMemOp);
  decode.waitOnChannel(decToExWOCHE); execute.waitOnChannelIn(decToExWOCHE);
  decode.predicate(readPredSig);      pred.output(readPredSig);
  decode.setPredicate(setPredSig);    execute.setPredicate(setPredSig);
  decode.stallOut(decStallSig);

  // To/from execute stage
  execute.output(ALUOutput);          execute.fromALU1(ALUOutput);
  execute.fromALU2(ALUOutput);
  write.fromALU(ALUOutput);

  execute.predicate(writePredSig);    pred.write(writePredSig);

  execute.remoteInstOut(exToWriteInst);     write.inst(exToWriteInst);
  execute.remoteChannelOut(exToWriteRChan); write.remoteChannel(exToWriteRChan);
  execute.memoryOpOut(exToWriteMemOp);      write.memoryOp(exToWriteMemOp);
  execute.waitOnChannelOut(exToWriteWOCHE); write.waitOnChannel(exToWriteWOCHE);

  execute.writeOut(writeAddr);        write.inRegAddr(writeAddr);
  execute.indWriteOut(indWriteAddr);  write.inIndAddr(indWriteAddr);

  // To/from write stage
  write.outRegAddr(writeRegAddr);     regs.writeAddr(writeRegAddr);
  write.outIndAddr(indirectWrite);    regs.indWriteAddr(indirectWrite);
  write.regData(regWriteData);        regs.writeData(regWriteData);
  write.stallOut(writeStallSig);

  end_module(); // Needed because we're using a different Component constructor
}

Cluster::~Cluster() {

}
