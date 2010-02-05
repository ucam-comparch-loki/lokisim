/*
 * DecodeStageTest.cpp
 *
 *  Created on: 4 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Pipeline/Decode/DecodeStage.h"
#include "../Memory/IndirectRegisterFile.h"

class DecodeStageTest : public ::testing::Test {
protected:

  DecodeStage ds;
  IndirectRegisterFile regs;

  sc_clock clock;

  sc_signal<Word> in1, in2, regOutput1, regOutput2;
  sc_signal<AddressedWord> out;
  sc_signal<Instruction> instIn, instOut;
  sc_signal<bool> cacheHit, isIndirect;
  sc_signal<Address> address;
  sc_signal<short> regRead1, regRead2, indRead, write, indWrite;
  sc_signal<short> operation, op1Select, op2Select;
  sc_signal<Data> RCETtoALU1, RCETtoALU2, regToALU1, regToALU2, SEtoALU;

  // Should come from WriteStage, but don't have one of those here
  sc_signal<short> write2, indWrite2;
  sc_signal<Word> writeData;

  DecodeStageTest() : ds("decode", 1), regs("regs", 1),
      clock("clock", 1, SC_NS, 0.5) {

    ds.address(address);
    ds.cacheHit(cacheHit);
    ds.chEnd1(RCETtoALU1);
    ds.chEnd2(RCETtoALU2);
    ds.clock(clock);
    ds.in1(in1);
    ds.in2(in2);
    ds.isIndirect(isIndirect); regs.indRead(isIndirect);
    ds.indReadAddr(indRead);
    ds.indWriteAddr(indWrite);
    ds.inst(instIn);
    ds.remoteInst(instOut);
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
    ds.sExtend(SEtoALU);
    ds.writeAddr(write);

    regs.writeAddr(write2);
    regs.indWriteAddr(indWrite2);
    regs.writeData(writeData);
  }

  virtual void SetUp() {

    Word w1(1), w2(2), w3(3);

  // Put some data in the registers
    write2.write(2);
    writeData.write(w2);

    TIMESTEP;

    write2.write(8);
    writeData.write(w3);

    TIMESTEP;

    write2.write(19);
    writeData.write(w1);

    TIMESTEP;

    // Put stuff in RCET?
  }

// Also want to test that it can do fetching, but need FetchLogic working for that

};

TEST_F(DecodeStageTest, PreparesOperands) {

  Instruction i1("addi r3 r2 14"), i2("sll r4 r8 r19");
  Data d1, d2;
  Instruction temp;

  instIn.write(i1);

  TIMESTEP;

  d1 = regToALU1.read(); d2 = SEtoALU.read(); temp = instOut.read();
  EXPECT_EQ(2, d1.getData());
  EXPECT_EQ(14, d2.getData());
  EXPECT_EQ(1, op1Select.read());
  EXPECT_EQ(3, op2Select.read());
  EXPECT_EQ(3, write.read());
  EXPECT_EQ(i1, temp);
  EXPECT_EQ(false, isIndirect.read());

  instIn.write(i2);

  TIMESTEP;

  d1 = regToALU1.read(); d2 = regToALU2.read(); temp = instOut.read();
  EXPECT_EQ(3, d1.getData());
  EXPECT_EQ(1, d2.getData());
  EXPECT_EQ(1, op1Select.read());
  EXPECT_EQ(1, op2Select.read());
  EXPECT_EQ(4, write.read());
  EXPECT_EQ(i2, temp);
  EXPECT_EQ(false, isIndirect.read());

}
