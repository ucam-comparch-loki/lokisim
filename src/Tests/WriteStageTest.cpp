/*
 * WriteStageTest.cpp
 *
 *  Created on: 8 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Pipeline/Write/WriteStage.h"
#include "../Memory/IndirectRegisterFile.h"

//TEST(WriteStageTest, WriteStageWorks) {
//
//  WriteStage write("write", 1);
//  IndirectRegisterFile regs("regs", 1);
//
//  sc_clock clock("clock", 1, SC_NS, 0.5);
//
//  sc_signal<Data> dataIn;
//  sc_signal<Instruction> instIn;
//  sc_signal<short> regAddrIn, regAddrOut, indAddrIn, indAddrOut, regRead1,
//                   regRead2, remoteChannel;
//  sc_signal<Word> regData, regOut1, regOut2;
//  sc_signal<Array<AddressedWord> > output;
//  sc_signal<bool> indRead;
//  sc_signal<Array<bool> > flowControl;
//
//  write.clock(clock);
//  write.fromALU(dataIn);
//  write.inst(instIn);
//  write.inRegAddr(regAddrIn);
//  write.inIndAddr(indAddrIn);
//  write.outRegAddr(regAddrOut); regs.writeAddr(regAddrOut);
//  write.outIndAddr(indAddrOut); regs.indWriteAddr(indAddrOut);
//  write.regData(regData); regs.writeData(regData);
//  write.remoteChannel(remoteChannel);
//  write.flowControl(flowControl);
//  write.output(output);
//
//  regs.indRead(indRead);
//  regs.readAddr1(regRead1);
//  regs.readAddr2(regRead2);
//  regs.out1(regOut1);
//  regs.out2(regOut2);
//
//  Data d1(1);
//  Instruction i1("fetch r2 4");
//  Array<bool> control(2);
//  bool t=true, f=false;
//  control.put(0, f); control.put(1, f);
//  Word temp;
//  Array<AddressedWord> temp2;
//
//  regAddrIn.write(3);
//  remoteChannel.write(0);
//  dataIn.write(d1);             // Put d1 in register 3 and channel-end 0
//  flowControl.write(control);
//
//  TIMESTEP;
//
//  remoteChannel.write(1);
//  instIn.write(i1);             // Put i1 in channel-end 1
//
//  TIMESTEP;
//
//  control.put(0, t);            // Allow data to be sent from channel-end 0
//  flowControl.write(control);
//  regRead1.write(3);
//
//  TIMESTEP;
//
//  temp = regOut1.read();
//  temp2 = output.read();
//  EXPECT_EQ(d1, (Data)temp);
//  EXPECT_EQ(d1, temp2.get(0).getPayload());
//  EXPECT_EQ(0, temp2.get(0).getChannelID());
//  control.put(1, t);            // Allow data to be sent from channel-end 1
//  flowControl.write(control);
//
//  TIMESTEP;
//
//  temp2 = output.read();
//  EXPECT_EQ(i1, temp2.get(1).getPayload());
//  EXPECT_EQ(1, temp2.get(1).getChannelID());
//
//}
