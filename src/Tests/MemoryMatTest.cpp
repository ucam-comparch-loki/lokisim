/*
 * MemoryMatTest.cpp
 *
 *  Created on: 4 Mar 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../TileComponents/MemoryMat.h"
#include "../Utility/CodeLoader.h"

class MemoryMatTest : public ::testing::Test {
protected:

  MemoryMat                   m;

  sc_clock                    clock;
  flag_signal<Word>          *in;
  flag_signal<AddressedWord> *out;
  sc_signal<bool>            *fcIn;
  sc_signal<bool>            *fcOut;

  MemoryMatTest() :
      m("memory", 1),
      clock("clock", 1, SC_NS, 0.5) {

    in    = new flag_signal<Word>[NUM_CLUSTER_INPUTS];
    out   = new flag_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];
    fcIn  = new sc_signal<bool>[NUM_CLUSTER_OUTPUTS];
    fcOut = new sc_signal<bool>[NUM_CLUSTER_INPUTS];

  }

  virtual void SetUp() {
    m.clock(clock);
    for(int i=0; i<NUM_CLUSTER_INPUTS; i++) {
      m.in[i](in[i]);
      m.flowControlOut[i](fcOut[i]);
    }
    for(int i=0; i<NUM_CLUSTER_OUTPUTS; i++) {
      m.out[i](out[i]);
      m.flowControlIn[i](fcIn[i]);
    }
  }

  virtual void TearDown() {

  }

};

//TEST_F(MemoryMatTest, MemoryMatWorks) {
//
//  AddressedWord temp;
//
//  Data d1(1), d2(2), d3(3);
//  MemoryRequest write(0, 100, 3, false), read(0, 100, 3, true);
//
//  fcIn[2].write(true);
//
//  // Put some data in the memory
//  in[2].write(write);
//  TIMESTEP;
//  in[2].write(d1);
//  TIMESTEP;
//  in[2].write(d2);
//  TIMESTEP;
//  in[2].write(d3);
//  TIMESTEP;
//
//  // Read the data from memory
//  in[2].write(read);
//  TIMESTEP;
//  temp = out[2].read();
//  EXPECT_EQ(d1, temp.getPayload());
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//  temp = out[2].read();
//  EXPECT_EQ(d2, temp.getPayload());
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//  temp = out[2].read();
//  EXPECT_EQ(d3, temp.getPayload());
//  EXPECT_EQ(100, temp.getChannelID());
//
//}

/* Tests that is is possible to read a whole instruction packet with a
 * single request. */
//TEST_F(MemoryMatTest, InstructionPacket) {
//
//  // Load an instruction packet into the memory
//  char* code = "/home/db434/Documents/Simulator/Test Code/fibonacci.loki";
//  CodeLoader::loadCode(code, m);
//
//  AddressedWord temp;
//  Instruction i;
//
//  MemoryRequest read(0, 100, 0, true);
//  read.setIPKRequest(true);
//
//  fcIn[2].write(true);
//
//  // Send the request
//  in[2].write(read);
//  TIMESTEP;
//
//  // Check the outputs
//  temp = out[2].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  ASSERT_EQ(0, i.getRchannel());
//  ASSERT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//  temp = out[2].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  ASSERT_EQ(1, i.getRchannel()) << "Stopped reading after one flit.";
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//  temp = out[2].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  EXPECT_EQ(2, i.getRchannel());
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//  temp = out[2].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  EXPECT_EQ(3, i.getRchannel());
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//  temp = out[2].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  EXPECT_EQ(4, i.getRchannel());
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//  temp = out[2].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  EXPECT_EQ(5, i.getRchannel());
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//}
