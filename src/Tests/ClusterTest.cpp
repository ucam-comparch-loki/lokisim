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


/* Tests that Clusters are implemented fully enough to be instantiated */
//TEST(ClusterTest, CanCreateCluster) {
//
//  Cluster c("cluster", 1);
//
//  // If execution gets this far, the cluster has been instantiated
//  EXPECT_STREQ("cluster", c.name());
//
//}

/* Tests that Clusters are capable of all stages of execution, in a simple case */
//TEST(ClusterTest, ClusterExecutesInstructions) {
//
//  Cluster c("cluster", 1);
//
//  sc_clock clock("clock", 10, SC_NS, 0.5);
//
//  Instruction store5("addui 1 2 5");  // Add 5 to r2 (0) and store in r1
//  Instruction store7("addui 2 2 7");  // Add 7 to r2 (0) and store in r2
//  Instruction add("addu 3 1 2");      // Add 5 and 7 and store in r3
//
//  sc_signal<Word> toCache, toFIFO, toRCET1, toRCET2;
//  sc_signal<Array<AddressedWord> > output;
//
//  c.in1(toFIFO);
//  c.in2(toCache);
//  c.in3(toRCET1);
//  c.in4(toRCET2);
//  c.out(output);
//
//  toCache.write(store5);
//  clock.start(1.0, SC_NS);
//  toCache.write(store7);
//  clock.start(1.0, SC_NS);
//  toCache.write(add);
//  clock.start(10.0, SC_NS);
//
//}
