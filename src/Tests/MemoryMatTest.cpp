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
#include "../Datatype/ChannelRequest.h"
#include "../Datatype/MemoryRequest.h"

class MemoryMatTest : public ::testing::Test {
protected:

  MemoryMat                   memory;

  sc_clock                    clock;
  flag_signal<Word>          *in;
  flag_signal<AddressedWord> *out;
  sc_signal<bool>            *fcIn;
  sc_signal<bool>            *fcOut;

  MemoryMatTest() :
      memory("memory", 1),
      clock("clock", 1, SC_NS, 0.5) {

    in    = new flag_signal<Word>[NUM_CLUSTER_INPUTS];
    out   = new flag_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];
    fcIn  = new sc_signal<bool>[NUM_CLUSTER_OUTPUTS];
    fcOut = new sc_signal<bool>[NUM_CLUSTER_INPUTS];

  }

  virtual void SetUp() {
    memory.clock(clock);
    for(int i=0; i<NUM_CLUSTER_INPUTS; i++) {
      memory.in[i](in[i]);
      memory.flowControlOut[i](fcOut[i]);
    }
    for(int i=0; i<NUM_CLUSTER_OUTPUTS; i++) {
      memory.out[i](out[i]);
      memory.flowControlIn[i](fcIn[i]);
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
//  MemoryRequest write1(1,0,0,false), write2(2,0,0,false), write3(3,0,0,false);
//  MemoryRequest read1(1,0,0,true),   read2(2,0,0,true),   read3(3,0,0,true);
//  ChannelRequest setup(1, 100, ChannelRequest::SETUP),
//                 teardown(1, 100, ChannelRequest::TEARDOWN);
//
//  fcIn[1].write(true);
//
//  // Set up the connection
//  in[MemoryMat::CONTROL_INPUT].write(setup);
//  TIMESTEP;
//    // Check for ACK
//
//  // Put some data in the memory
//  in[1].write(write1);
//  TIMESTEP;
//  in[1].write(d1);
//  TIMESTEP;
//  in[1].write(write2);
//  TIMESTEP;
//  in[1].write(d2);
//  TIMESTEP;
//  in[1].write(write3);
//  TIMESTEP;
//  in[1].write(d3);
//  TIMESTEP;
//
//  // Read the data from memory
//  in[1].write(read1);
//  TIMESTEP;
//  temp = out[1].read();
//  EXPECT_EQ(d1, temp.getPayload());
//  EXPECT_EQ(100, temp.getChannelID());
//  in[1].write(read2);
//  TIMESTEP;
//  temp = out[1].read();
//  EXPECT_EQ(d2, temp.getPayload());
//  EXPECT_EQ(100, temp.getChannelID());
//  in[1].write(read3);
//  TIMESTEP;
//  temp = out[1].read();
//  EXPECT_EQ(d3, temp.getPayload());
//  EXPECT_EQ(100, temp.getChannelID());
//
//  // Tear down the connection
//  in[MemoryMat::CONTROL_INPUT].write(teardown);
//  TIMESTEP;
//    // Check that it worked
//
//}

/* Tests that is is possible to read a whole instruction packet with a
 * single request. */
//TEST_F(MemoryMatTest, InstructionPacket) {
//
//  // Load an instruction packet into the memory
//  string code = "fibonacci.loki";
//  CodeLoader::loadCode(code, memory);
//
//  AddressedWord temp;
//  Instruction i;
//
//  ChannelRequest setup(1, 100, ChannelRequest::SETUP),
//                 teardown(1, 100, ChannelRequest::TEARDOWN);
//
//  MemoryRequest read(0, 100, 0, true);
//  read.setIPKRequest(true);
//
//  fcIn[1].write(true);
//
//  // Set up the channel
//  in[MemoryMat::CONTROL_INPUT].write(setup);
//  TIMESTEP;
//
//  // Send the request
//  in[1].write(read);
//  TIMESTEP;
//
//  // Check the outputs
//  temp = out[1].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  ASSERT_EQ(0, i.getRchannel());
//  ASSERT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//  temp = out[1].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  ASSERT_EQ(1, i.getRchannel()) << "Stopped reading after one flit.";
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//  temp = out[1].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  EXPECT_EQ(2, i.getRchannel());
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//  temp = out[1].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  EXPECT_EQ(3, i.getRchannel());
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//  temp = out[1].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  EXPECT_EQ(4, i.getRchannel());
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//  temp = out[1].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  EXPECT_EQ(5, i.getRchannel());
//  EXPECT_EQ(100, temp.getChannelID());
//  TIMESTEP;
//
//  temp = out[1].read();
//  i = static_cast<Instruction>(temp.getPayload());
//  ASSERT_EQ(5, i.getRchannel()) << "Didn't stop at end of packet.";
//  EXPECT_EQ(100, temp.getChannelID());
//
//  in[MemoryMat::CONTROL_INPUT].write(teardown);
//  TIMESTEP;
//
//}
