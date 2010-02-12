/*
 * FetchLogicTest.cpp
 *
 *  Created on: 3 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Pipeline/Decode/FetchLogic.h"

//TEST(FetchLogicTest, FetchLogicWorks) {
//
//// Create components
//  FetchLogic fl("fetchlogic", 1);
//
//  sc_signal<Address> input, toIPKC;
//  sc_signal<bool> cacheHit, roomToFetch, flowControl;
//  sc_signal<AddressedWord> output;
//  sc_signal<Data> baseAddress;
//
//// Connect things up
//  fl.in(input);
//  fl.cacheContainsInst(cacheHit);
//  fl.isRoomToFetch(roomToFetch);
//  fl.flowControl(flowControl);
//  fl.toIPKC(toIPKC);
//  fl.toNetwork(output);
//  fl.baseAddress(baseAddress);
//
//  // Need to use isRoomToFetch too
//
//// Test
//  Address a1(1,1), a2(2,2);
//  AddressedWord temp, temp2;
//  Data d(1);
//
//  baseAddress.write(d);
//  input.write(a1);
//
//  TIMESTEP;
//
//  cacheHit.write(true);     // Already have the instruction packet in cache
//
//  TIMESTEP;
//
//  temp = output.read();
//  EXPECT_EQ(0, temp.getAddress()); // The output shouldn't have a value yet
//  input.write(a2);
//
//  TIMESTEP;
//
//  cacheHit.write(false);    // Want to fetch the packet
//
//  TIMESTEP;
//
//  temp = output.read();
////  EXPECT_EQ(temp.getPayload());
//  EXPECT_EQ(1, temp.getChannelID());
//  EXPECT_EQ(a2.getAddress()+1, temp.getAddress());
//
//  TIMESTEP;
//
//  temp2 = output.read();
//  EXPECT_EQ(temp, temp2);   // Shouldn't send next flit until flow control changes
//  flowControl.write(!flowControl.read());
//
//  TIMESTEP;
//
//  temp = output.read();
//  EXPECT_EQ(a2.getAddress()+1, ((Address)(temp.getPayload())).getAddress());
//  EXPECT_EQ(a2.getAddress()+1, temp.getAddress());
//  EXPECT_EQ(1, temp.getChannelID());
//
//}
