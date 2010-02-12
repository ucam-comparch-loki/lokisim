/*
 * FetchStageTest.cpp
 *
 *  Created on: 3 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Pipeline/Fetch/FetchStage.h"

//TEST(FetchStageTest, FetchStageWorks) {
//
//  FetchStage fs("fetch", 1);
//
//  sc_clock clock("clock", 1, SC_NS, 0.5);
//  sc_signal<Word> in1, in2;
//  sc_signal<Address> address;
//  sc_signal<Instruction> out;
//  sc_signal<bool> cacheHit, roomToFetch;
//
//  fs.clock(clock);
//  fs.toIPKQueue(in1);
//  fs.toIPKCache(in2);
//  fs.address(address);
//  fs.instruction(out);
//  fs.cacheHit(cacheHit);
//  fs.roomToFetch(roomToFetch);
//
//  Instruction i1("ld r4 16"), i2("ld r3 17"), i3("addu.eop r5 r3 r4");
//  Instruction i4("ld r14 16"), i5("ld r13 17"), i6("addu.eop r15 r13 r14");
//  Instruction i7("sll r4 r4 1"), i8("srl r4 r4 1"), i9("addu.eop r6 r4 r4");
//  Address a1(1,1), a2(2,2);
//
//  Instruction temp;
//
//  address.write(a1);
//
//  TIMESTEP;
//
//  in2.write(i1);
//
//  TIMESTEP;
//
//  in2.write(i2);
//  in1.write(i4);
//  temp = out.read();
//  EXPECT_EQ(i1, temp);
//
//  TIMESTEP;
//
//  in2.write(i3);
//  in1.write(i5);
//  address.write(a2);
//  temp = out.read();
//  EXPECT_EQ(i2, temp);
//
//  TIMESTEP;
//
//  in2.write(i7);
//  in1.write(i6);
//  temp = out.read();
//  EXPECT_EQ(false, cacheHit.read());
//  EXPECT_EQ(i3, temp);
//
//  TIMESTEP;
//
//  in2.write(i8);
//  temp = out.read();
//  EXPECT_EQ(i4, temp) << "Didn't switch from cache to FIFO.";
//
//  TIMESTEP;
//
//  in2.write(i9);
//  temp = out.read();
//  EXPECT_EQ(i5, temp) << "Switched back to cache too quickly.";
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(i6, temp);
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(i7, temp) << "Didn't switch from FIFO to cache.";
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(i8, temp);
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(i9, temp);
//
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//
//  in2.write(i3);      // See if it starts sending new instructions immediately
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(i3, temp) << "Cache doesn't send new Instructions when empty.";
//
//}
