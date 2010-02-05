/*
 * IPKFIFOTest.cpp
 *
 *  Created on: 2 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Pipeline/Fetch/InstructionPacketFIFO.h"

//TEST(IPKFIFOTest, IPKFIFOWorks) {
//
//  InstructionPacketFIFO fifo("fifo", 1);
//
//  sc_signal<bool> readSig, empty;
//  sc_signal<Instruction> in, out;
//
//  fifo.readInstruction(readSig);
//  fifo.empty(empty);
//  fifo.in(in);
//  fifo.out(out);
//
//  Instruction i1(1), i2(2), i3(3);
//  Instruction temp;
//
//
//  in.write(i1);
//
//  TIMESTEP;
//
//  EXPECT_EQ(false, empty.read());
//  in.write(i2);
//
//  TIMESTEP;
//
//  in.write(i3);
//  readSig.write(!readSig.read());
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(i1, temp);
//  readSig.write(!readSig.read());
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(i2, temp);;
//  readSig.write(!readSig.read());
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(i3, temp);
//  EXPECT_EQ(true, empty.read());
//
//}
