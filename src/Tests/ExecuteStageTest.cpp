/*
 * ExecuteStageTest.cpp
 *
 *  Created on: 28 Jan 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include <systemc.h>
#include "Test.h"
#include "../Pipeline/Execute/ExecuteStage.h"
#include "../InstructionMap.h"

//TEST(ExecuteStageTest, ExecuteStageWorks) {
//
//// Create and connect up the component
//  ExecuteStage execute("execute", 1);
//  sc_clock clock("clock", 1, SC_NS, 0.5);
//
//  sc_signal<Data> mux1_1, mux1_2, mux1_3, mux2_1, mux2_2, mux2_3, mux2_4, out;
//  sc_signal<short> mux1_select, mux2_select, operation;
//  sc_signal<short> writeAddr, indWriteAddr, rChannel;
//  sc_signal<Instruction> remoteInst;
//
//  execute.clock(clock);
//
//  execute.fromRChan1(mux1_1);
//  execute.fromReg1(mux1_2);
//  execute.fromALU1(mux1_3);
//  execute.fromRChan2(mux2_1);
//  execute.fromReg2(mux2_2);
//  execute.fromALU2(mux2_3);
//  execute.fromSExtend(mux2_4);
//
//  execute.op1Select(mux1_select);
//  execute.op2Select(mux2_select);
//  execute.operation(operation);
//
//  execute.output(out);
//
//// Unused inputs/outputs (connecting them to themselves because I'm lazy)
//  execute.writeIn(writeAddr); execute.writeOut(writeAddr);
//  execute.indWriteIn(indWriteAddr); execute.indWriteOut(indWriteAddr);
//  execute.remoteInstIn(remoteInst); execute.remoteInstOut(remoteInst);
//  execute.remoteChannelIn(rChannel); execute.remoteChannelOut(rChannel);
//
//// Prepare initial values
//  Data d1(1), d2(2), d3(3), d4(4), d5(5), d6(6), d7(7);
//
//  mux1_1.write(d1);
//  mux1_2.write(d2);
//  mux1_3.write(d3);
//  mux2_1.write(d4);
//  mux2_2.write(d5);
//  mux2_3.write(d6);
//  mux2_4.write(d7);
//
//// Begin testing
//  mux1_select.write(1); mux2_select.write(1);
//  operation.write(InstructionMap::ADDU);    // 2 + 5 = 7
//
//  sc_start(clock.period());
//
//  Data d = out.read();
//  EXPECT_EQ(7, d.getData());
//
//  mux1_select.write(2); mux2_select.write(3);
//  operation.write(InstructionMap::AND);      // 3 & 7 = 3
//
//  sc_start(clock.period());
//
//  d = out.read();
//  EXPECT_EQ(3, d.getData());
//
//}
