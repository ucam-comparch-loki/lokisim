/*
 * ClusterTest.cpp
 *
 *  Created on: 26 Jan 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../TileComponents/Cluster.h"
#include "../Utility/CodeLoader.h"
#include "../flag_signal.h"

class ClusterTest : public ::testing::Test {
protected:

  Cluster c;

  sc_clock clock;
  flag_signal<Word> *in;            // array
  flag_signal<AddressedWord> *out;  // array
  sc_buffer<bool> *flowControlIn;   // array
  sc_buffer<bool> *flowControlOut;  // array

  sc_core::sc_trace_file *trace;

  ClusterTest() :
      c("cluster", 1),
      clock("clock", 1, SC_NS, 0.5) {

    in = new flag_signal<Word>[NUM_CLUSTER_INPUTS];
    out = new flag_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];
    flowControlIn = new sc_buffer<bool>[NUM_CLUSTER_OUTPUTS];
    flowControlOut = new sc_buffer<bool>[NUM_CLUSTER_INPUTS];

    c.clock(clock);
    for(int i=0; i<NUM_CLUSTER_INPUTS; i++) {
      c.in[i](in[i]);
      c.flowControlOut[i](flowControlOut[i]);
    }
    for(int i=0; i<NUM_CLUSTER_OUTPUTS; i++) {
      c.out[i](out[i]);
      c.flowControlIn[i](flowControlIn[i]);
    }

  }

  virtual void SetUp() {
    string filename("ClusterTest");
    trace = TraceFile::initialiseTraceFile(filename);

    for(int i=0; i<NUM_CLUSTER_OUTPUTS; i++) {
      flowControlIn[i].write(false);

      std::string name(c.out[i].name());
      sc_core::sc_trace(trace, out[i], name);
    }
  }

  virtual void TearDown() {
    TIMESTEP;
    sc_core::sc_close_vcd_trace_file(trace);
  }

};

/* Tests that Clusters are implemented fully enough to be instantiated */
//TEST_F(ClusterTest, CanCreateCluster) {
//
//  TIMESTEP;
//
//  // If execution gets this far, the cluster has been instantiated correctly
//  EXPECT_STREQ("cluster_1", c.name());
//
//}

/* Tests that Clusters are capable of all stages of execution, in a simple case */
//TEST_F(ClusterTest, ClusterExecutesFibonacci) {
//
//  // Note: Only works if all instructions are allowed to specify remote channels
//
//  Instruction store1("addui r1 r0 1 > 0"), store2("addui r2 r0 1 > 1"),
//              add1("addu r3 r2 r1 > 2"), add2("addu r4 r3 r2 > 3"),
//              add3("addu r5 r4 r3 > 4"), add4("addu r6 r5 r4 > 5"),
//              fetch("fetch r0 4");
//
//  Array<AddressedWord> temp;
//
//
//  in[1].write(fetch);
//  TIMESTEP;
//  in[1].write(store1);
//  TIMESTEP;
//  in[1].write(store2);
//  TIMESTEP;
//  in[1].write(add1);
//  TIMESTEP;
//  in[1].write(add2);
//  TIMESTEP;
//  in[1].write(add3);
//  TIMESTEP;
//  in[1].write(add4);
//  TIMESTEP;
//
//  flowControlIn[1].write(true);
//
//  TIMESTEP;
//
//  EXPECT_EQ(1, ((Data)out[1].read().getPayload()).getData());
//  EXPECT_EQ(0, out[1].read().getChannelID());
//
//  flowControlIn[1].write(true);
//  flowControlIn[2].write(true);
//
//  TIMESTEP;
//
//  EXPECT_EQ(1, ((Data)out[2].read().getPayload()).getData());
//  EXPECT_EQ(1, out[2].read().getChannelID());
//  EXPECT_EQ(2, ((Data)out[1].read().getPayload()).getData());
//  EXPECT_EQ(2, out[1].read().getChannelID());
//
//  flowControlIn[1].write(true);
//  flowControlIn[2].write(true);
//
//  TIMESTEP;
//
//  EXPECT_EQ(3, ((Data)out[2].read().getPayload()).getData());
//  EXPECT_EQ(3, out[2].read().getChannelID());
//  EXPECT_EQ(5, ((Data)out[1].read().getPayload()).getData());
//  EXPECT_EQ(4, out[1].read().getChannelID());
//
//  flowControlIn[1].write(true);
//  flowControlIn[2].write(true);
//
//  TIMESTEP;
//
//  EXPECT_EQ(8, ((Data)out[2].read().getPayload()).getData());
//  EXPECT_EQ(5, out[2].read().getChannelID());
//  EXPECT_EQ(5, ((Data)out[1].read().getPayload()).getData());
//  EXPECT_EQ(4, out[1].read().getChannelID());
//
//}

/* Tests that it is possible to load in code from a file, and execute it */
//TEST_F(ClusterTest, RunsExternalCode) {
//
//  std::string filename = "fibonacci.loki";
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
//  flowControlIn[1].write(true);
//
//  TIMESTEP;
//
//  EXPECT_EQ(1, ((Data)(out[1].read().getPayload())).getData());
//  EXPECT_EQ(0, out[1].read().getChannelID());
//  EXPECT_EQ(0, ((Data)(out[2].read().getPayload())).getData())
//    << "Data being sent before being allowed to by flow control.";
//  EXPECT_EQ(0, out[2].read().getChannelID());
//
//  flowControlIn[2].write(true);
//
//  TIMESTEP;
//
//  EXPECT_EQ(1, ((Data)(out[2].read().getPayload())).getData());
//  EXPECT_EQ(1, out[2].read().getChannelID());
//  EXPECT_EQ(2, ((Data)(out[1].read().getPayload())).getData());
//  EXPECT_EQ(2, out[1].read().getChannelID());
//
//  TIMESTEP;
//
//  EXPECT_EQ(3, ((Data)(out[2].read().getPayload())).getData());
//  EXPECT_EQ(3, out[2].read().getChannelID());
//  EXPECT_EQ(5, ((Data)(out[1].read().getPayload())).getData());
//  EXPECT_EQ(4, out[1].read().getChannelID());
//
//  TIMESTEP;
//
//  EXPECT_EQ(8, ((Data)(out[2].read().getPayload())).getData());
//  EXPECT_EQ(5, out[2].read().getChannelID());
//  EXPECT_EQ(5, ((Data)(out[1].read().getPayload())).getData());
//  EXPECT_EQ(4, out[1].read().getChannelID());
//
//}

/* Loads in code from a file, but just prints anything sent from the cluster.
 * Google Test will always say this test passes, as there are no assertions,
 * so the user should determine whether code executes correctly themselves. */
//TEST_F(ClusterTest, ExternalCode) {
//
//  std::string filename = "fibonacci.loki";
//  int setupCycles = 5;    // Number of cycles before outputs should be printed
//  int numCycles = 5;      // Number of cycles for outputs to be printed
//
//  CodeLoader::loadCode(filename, c);
//
//  Array<AddressedWord> temp;
//
//  sc_start(setupCycles * clock.period());
//
//  for(int i=0; i<numCycles; i++) {
//
//    TIMESTEP;
//
//    std::cout << "Cycle " << i << ":\n";
//
//    for(int j=0; j<NUM_CLUSTER_OUTPUTS; j++) {
//      std::cout << "  out" << j << ": " << out[j].read() << "\n";
//      flowControlIn[j].write(true);
//    }
//  }
//
//}

/* Tests that filling a Send Channel-end Table buffer with data causes the
 * pipeline to stall, and that removing data from the buffer allows the
 * pipeline to continue execution again. */
//TEST_F(ClusterTest, SCETStallsPipeline) {
//
//  std::string filename = "fibonacci2.loki";
//
//  CodeLoader::loadCode(filename, c);
//
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//
//  // This should be enough cycles that the buffer has filled up, so the
//  // pipeline should have stalled.
//
//  ASSERT_EQ(0, ((Data)(out[1].read().getPayload())).getData())
//    << "Data escaped from send channel-end table too soon.";
//
//  flowControlIn[1].write(true);
//
//  TIMESTEP;
//
//  ASSERT_EQ(1, ((Data)(out[1].read().getPayload())).getData());
//  EXPECT_EQ(6, out[1].read().getChannelID());
//
//  TIMESTEP;
//
//  EXPECT_EQ(1, ((Data)(out[1].read().getPayload())).getData());
//  EXPECT_EQ(6, out[1].read().getChannelID());
//
//  TIMESTEP;
//
//  EXPECT_EQ(2, ((Data)(out[1].read().getPayload())).getData());
//  EXPECT_EQ(6, out[1].read().getChannelID());
//
//  TIMESTEP;
//
//  EXPECT_EQ(3, ((Data)(out[1].read().getPayload())).getData());
//  EXPECT_EQ(6, out[1].read().getChannelID());
//
//  TIMESTEP;
//
//  ASSERT_EQ(5, ((Data)(out[1].read().getPayload())).getData())
//    << "Cluster didn't restart execution once buffer had space.";
//  EXPECT_EQ(6, out[1].read().getChannelID());
//
//  TIMESTEP;
//
//  EXPECT_EQ(8, ((Data)(out[1].read().getPayload())).getData());
//  EXPECT_EQ(6, out[1].read().getChannelID());
//
//}

/* Tests that reads to the receive channel-end table are blocking (the
 * pipeline stalls if the buffer being read is empty) and that the cluster
 * resumes execution when data arrives. */
//TEST_F(ClusterTest, RCETStallsPipeline) {
//
//  Instruction i("addui r1 r28 3 > 2");  // r28 = receive channel 0
//  Data d(2);
//
//  flowControlIn[1].write(true);
//  in[1].write(i);
//
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//
//  // Pipeline should have stalled waiting for data
//
//  EXPECT_EQ(0, ((Data)(out[1].read().getPayload())).getData())
//    << "Cluster didn't wait for data.";
//  in[2].write(d);
//
//  TIMESTEP;
//  TIMESTEP;
//  TIMESTEP;
//
//  EXPECT_EQ(5, ((Data)(out[1].read().getPayload())).getData());
//  EXPECT_EQ(2, out[1].read().getChannelID());
//
//}
