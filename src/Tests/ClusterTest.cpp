/*
 * ClusterTest.cpp
 *
 *  Created on: 26 Jan 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "systemc.h"
#include "Test.h"
#include "../Cluster.h"
#include "../CodeLoader.h"

class ClusterTest : public ::testing::Test {
protected:

  Cluster c;

  sc_clock clock;
  sc_signal<Word> in1, in2, in3, in4;
  sc_buffer<bool>* flowControlIn;   // array
  sc_signal<AddressedWord>* out;    // array

  ClusterTest() :
      c("cluster", 1),
      clock("clock", 1, SC_NS, 0.5) {

    flowControlIn = new sc_buffer<bool>[NUM_CLUSTER_OUTPUTS];
    out = new sc_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];

    c.clock(clock);
    c.in1(in1);
    c.in2(in2);
    c.in3(in3);
    c.in4(in4);
    for(int i=0; i<NUM_CLUSTER_OUTPUTS; i++) {
      c.flowControlIn[i](flowControlIn[i]);
      c.out[i](out[i]);
    }

  }

  virtual void SetUp() {
    for(int i=0; i<NUM_CLUSTER_OUTPUTS; i++) flowControlIn[i].write(false);
  }

};

/* Tests that Clusters are implemented fully enough to be instantiated */
//TEST_F(ClusterTest, CanCreateCluster) {
//
//  TIMESTEP;
//
//  // If execution gets this far, the cluster has been instantiated correctly
//  EXPECT_STREQ("cluster", c.name());
//
//}

/* Tests that Clusters are capable of all stages of execution, in a simple case */
TEST_F(ClusterTest, ClusterExecutesFibonacci) {

  // Note: Only works if all instructions are allowed to specify remote channels

  Instruction store1("addui r1 r0 1 > 0"), store2("addui r2 r0 1 > 1"),
              add1("addu r3 r2 r1 > 2"), add2("addu r4 r3 r2 > 3"),
              add3("addu r5 r4 r3 > 4"), add4("addu r6 r5 r4 > 5"),
              fetch("fetch r0 4");

  // TODO: do more calculations to test stalling when buffers get full

  Array<AddressedWord> temp;


  in2.write(fetch);
  TIMESTEP;
  in2.write(store1);
  TIMESTEP;
  in2.write(store2);
  TIMESTEP;
  in2.write(add1);
  TIMESTEP;
  in2.write(add2);
  TIMESTEP;
  in2.write(add3);
  TIMESTEP;
  in2.write(add4);
  TIMESTEP;

  flowControlIn[1].write(true);

  TIMESTEP;

  EXPECT_EQ(1, ((Data)out[1].read().getPayload()).getData());
  EXPECT_EQ(0, out[1].read().getChannelID());

  flowControlIn[1].write(true);
  flowControlIn[2].write(true);

  TIMESTEP;

  EXPECT_EQ(1, ((Data)out[2].read().getPayload()).getData());
  EXPECT_EQ(1, out[2].read().getChannelID());
  EXPECT_EQ(2, ((Data)out[1].read().getPayload()).getData());
  EXPECT_EQ(2, out[1].read().getChannelID());

  flowControlIn[1].write(true);
  flowControlIn[2].write(true);

  TIMESTEP;

  EXPECT_EQ(3, ((Data)out[2].read().getPayload()).getData());
  EXPECT_EQ(3, out[2].read().getChannelID());
  EXPECT_EQ(5, ((Data)out[1].read().getPayload()).getData());
  EXPECT_EQ(4, out[1].read().getChannelID());

  flowControlIn[1].write(true);
  flowControlIn[2].write(true);

  TIMESTEP;

  EXPECT_EQ(8, ((Data)out[2].read().getPayload()).getData());
  EXPECT_EQ(5, out[2].read().getChannelID());
  EXPECT_EQ(5, ((Data)out[1].read().getPayload()).getData());
  EXPECT_EQ(4, out[1].read().getChannelID());

}

/* Tests that it is possible to load in code from a file, and execute it */
//TEST_F(ClusterTest, RunsExternalCode) {
//
//  char* filename = "/home/db434/Documents/Simulator/Test Code/fibonacci.loki";
//
//  CodeLoader::loadCode(filename, c);
//
//  Array<AddressedWord> temp;
//
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//
//  controlIn.put(1,t);
//  flowControlIn.write(controlIn);
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(1, ((Data)(temp.get(1).getPayload())).getData());
//  EXPECT_EQ(0, temp.get(1).getChannelID());
//
//  controlIn.put(2,t);
//  flowControlIn.write(controlIn);
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(1, ((Data)(temp.get(2).getPayload())).getData());
//  EXPECT_EQ(1, temp.get(2).getChannelID());
//  EXPECT_EQ(2, ((Data)(temp.get(1).getPayload())).getData());
//  EXPECT_EQ(2, temp.get(1).getChannelID());
//  flowControlIn.write(controlIn);
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(3, ((Data)(temp.get(2).getPayload())).getData());
//  EXPECT_EQ(3, temp.get(2).getChannelID());
//  EXPECT_EQ(5, ((Data)(temp.get(1).getPayload())).getData());
//  EXPECT_EQ(4, temp.get(1).getChannelID());
//  flowControlIn.write(controlIn);
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(8, ((Data)(temp.get(2).getPayload())).getData());
//  EXPECT_EQ(5, temp.get(2).getChannelID());
//  EXPECT_EQ(5, ((Data)(temp.get(1).getPayload())).getData());
//  EXPECT_EQ(4, temp.get(1).getChannelID());
//
//}

/* Loads in code from a file, but just prints anything sent from the cluster.
 * Google Test will always say this test passes, as there are no assertions,
 * but the user should determine whether code executes correctly themselves. */
//TEST_F(ClusterTest, ExternalCode) {
//
//  char* filename = "/home/db434/Documents/Simulator/Test Code/fibonacci.loki";
//  int setupCycles = 5;    // Number of cycles before outputs should be printed
//  int numCycles = 5;      // Number of cycles for outputs to be printed
//
//  CodeLoader::loadCode(filename, c);
//
//  Array<AddressedWord> temp;
//
//  sc_start(setupCycles * clock.period());
//
//  controlIn.put(1,t); controlIn.put(2,t);
//
//  for(int i=0; i<numCycles; i++) {
//    flowControlIn.write(controlIn);
//
//    TIMESTEP;
//
//    temp = out.read();
//
//    std::cout << "Cycle " << i << ":\n"
//              << "  out1: " << temp.get(1) << "\n"
//              << "  out2: " << temp.get(2) << std::endl;
//  }
//
//}
