/*
 * NetworkTest.cpp
 *
 *  Created on: 22 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Network/InterclusterNetwork.h"

//TEST(NetworkTest, NetworkSwitchesData) {
//
//  InterclusterNetwork network("network", 1);
//
//  int numInputs = NUM_CLUSTER_OUTPUTS * (CLUSTERS_PER_TILE + MEMS_PER_TILE);
//  int numOutputs = NUM_CLUSTER_INPUTS * (CLUSTERS_PER_TILE + MEMS_PER_TILE);
//
//  sc_signal<AddressedWord> *requestsIn = new sc_signal<AddressedWord>[numInputs];
//  sc_signal<AddressedWord> *responsesIn = new sc_signal<AddressedWord>[numInputs];
//  sc_signal<AddressedWord> *dataIn = new sc_signal<AddressedWord>[numInputs];
//  sc_signal<Word> *requestsOut = new sc_signal<Word>[numOutputs];
//  sc_signal<Word> *responsesOut = new sc_signal<Word>[numOutputs];
//  sc_signal<Word> *dataOut = new sc_signal<Word>[numOutputs];
//
//  for(int i=0; i<numInputs; i++) {
//    network.requestsIn[i](requestsIn[i]);
//    network.responsesIn[i](responsesIn[i]);
//    network.dataIn[i](dataIn[i]);
//  }
//  for(int i=0; i<numOutputs; i++) {
//    network.requestsOut[i](requestsOut[i]);
//    network.responsesOut[i](responsesOut[i]);
//    network.dataOut[i](dataOut[i]);
//  }
//
//  Data d1(1), d2(2), d3(3);
//  AddressedWord a1(d1, 1), a2(d2, 2), a3(d3, 3);
//  Word temp;
//
//  requestsIn[5].write(a1);
//  responsesIn[1].write(a2);
//  dataIn[9].write(a3);
//
//  TIMESTEP;
//
//  temp = requestsOut[1].read();
//  EXPECT_EQ(d1, temp);
//  temp = responsesOut[2].read();
//  EXPECT_EQ(d2, temp);
//  temp = dataOut[3].read();
//  EXPECT_EQ(d3, temp);
//
//}
