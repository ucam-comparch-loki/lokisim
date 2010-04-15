/*
 * DecodeStageTest.cpp
 *
 *  Created on: 4 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Datatype/MemoryRequest.h"
#include "../TileComponents/Pipeline/Decode/DecodeStage.h"
#include "../TileComponents/Pipeline/IndirectRegisterFile.h"
#include "../Utility/InstructionMap.h"

class DecodeStageTest : public ::testing::Test {
protected:

  DecodeStage ds;
  IndirectRegisterFile regs;

  sc_clock clock;

  sc_signal<Word> *in, regOutput1, regOutput2;
  sc_signal<AddressedWord> out;
  sc_signal<Instruction> instIn, instOut;
  sc_buffer<bool> cacheHit;
  sc_signal<bool> isIndirect, flowControl, roomToFetch, setPredicate, *fc;
  sc_signal<Address> address;
  sc_signal<short> regRead1, regRead2, write, indWrite, rChannel;
  sc_signal<short> operation, op1Select, op2Select;
  sc_signal<Data> RCETtoALU1, RCETtoALU2, regToALU1, regToALU2, SEtoALU;

  // Should come from WriteStage, but don't have one of those here
  sc_signal<short> write2, indWrite2;
  sc_signal<Word> writeData;

  DecodeStageTest() : ds("decode", 1), regs("regs"),
      clock("clock", 1, SC_NS, 0.5) {

    in = new sc_signal<Word>[NUM_RECEIVE_CHANNELS];
    fc = new sc_signal<bool>[NUM_RECEIVE_CHANNELS];

    for(int i=0; i<NUM_RECEIVE_CHANNELS; i++) {
      ds.in[i](in[i]);
      ds.flowControlOut[i](fc[i]);
    }

    ds.address(address);
    ds.cacheHit(cacheHit);
    ds.chEnd1(RCETtoALU1);
    ds.chEnd2(RCETtoALU2);
    ds.clock(clock);
    ds.flowControlIn(flowControl);
    ds.isIndirect(isIndirect); regs.indRead(isIndirect);
    ds.indWriteAddr(indWrite);
    ds.instructionIn(instIn);
    ds.remoteInst(instOut);
    ds.remoteChannel(rChannel);
    ds.op1Select(op1Select);
    ds.op2Select(op2Select);
    ds.operation(operation);
    ds.out1(out);
    ds.regIn1(regOutput1); regs.out1(regOutput1);
    ds.regIn2(regOutput2); regs.out2(regOutput2);
    ds.regOut1(regToALU1);
    ds.regOut2(regToALU2);
    ds.regReadAddr1(regRead1); regs.readAddr1(regRead1);
    ds.regReadAddr2(regRead2); regs.readAddr2(regRead2);
    ds.roomToFetch(roomToFetch);
    ds.sExtend(SEtoALU);
    ds.setPredicate(setPredicate);
    ds.writeAddr(write);

    regs.writeAddr(write2);
    regs.indWriteAddr(indWrite2);
    regs.writeData(writeData);
  }

  virtual void SetUp() {

    Word w1(1), w2(2), w3(3);

  // Put some data in the registers
    write2.write(12);
    writeData.write(w2);

    TIMESTEP;

    write2.write(18);
    writeData.write(w3);

    TIMESTEP;

    write2.write(19);
    writeData.write(w1);

    TIMESTEP;

    // Put stuff in RCET?

    // Initialise some inputs
    flowControl.write(true);
    roomToFetch.write(true);
  }

};

//TEST_F(DecodeStageTest, PreparesOperands) {
//
//  Instruction i1("addui r13 r12 14"), i2("sllv r14 r18 r19");
//  Data d1, d2;
//
//  instIn.write(i1);
//
//  TIMESTEP;
//
//  d1 = regToALU1.read(); d2 = SEtoALU.read();
//  EXPECT_EQ(2, d1.getData());
//  EXPECT_EQ(14, d2.getData());
//  EXPECT_EQ(InstructionMap::ADDUI, operation.read());
//  EXPECT_EQ(1, op1Select.read());
//  EXPECT_EQ(3, op2Select.read());
//  EXPECT_EQ(3, write.read());
//  EXPECT_EQ(false, isIndirect.read());
//
//  instIn.write(i2);
//
//  TIMESTEP;
//
//  d1 = regToALU1.read(); d2 = regToALU2.read();
//  EXPECT_EQ(3, d1.getData());
//  EXPECT_EQ(1, d2.getData());
//  EXPECT_EQ(InstructionMap::SLLV, operation.read());
//  EXPECT_EQ(1, op1Select.read());
//  EXPECT_EQ(1, op2Select.read());
//  EXPECT_EQ(4, write.read());
//  EXPECT_EQ(false, isIndirect.read());
//
//}

//TEST_F(DecodeStageTest, SendsFetchRequests) {
//
//  Instruction i1("fetch r12 4"), i2("setfetchch 18");
//
//  instIn.write(i2);
//
//  TIMESTEP;
//
//  instIn.write(i1);
//
//  TIMESTEP;
//
//  Address temp2 = address.read();
//  ASSERT_EQ(6, temp2.getAddress());
//  ASSERT_EQ(18, temp2.getChannelID());
//  cacheHit.write(false);    // Not in cache => should send request
//
//  TIMESTEP;
//
//  AddressedWord temp = out.read();
//  MemoryRequest mr = static_cast<MemoryRequest>(temp.getPayload());
//  EXPECT_EQ(6, mr.getAddress()); // 2 in r2 + 4 offset
//  EXPECT_EQ(1*NUM_CLUSTER_INPUTS + 1, mr.getReturnAddress());
//  EXPECT_EQ(18, temp.getChannelID());
//
//}
