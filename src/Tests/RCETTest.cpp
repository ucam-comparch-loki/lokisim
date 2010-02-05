/*
 * RCETTest.cpp
 *
 *  Created on: 29 Jan 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Pipeline/Decode/ReceiveChannelEndTable.h"

//TEST(RCETTest, RCETWorks) {
//
//// Create and connect up components
//  ReceiveChannelEndTable rcet("rcet", 1);
//
//  sc_buffer<Word> fromNetwork1, fromNetwork2;   // Data inputs
//  sc_buffer<short> fromDecoder1, fromDecoder2;  // Choose which channel to read
//  sc_buffer<Data> toALU1, toALU2;               // Data outputs
//
//  rcet.fromDecoder1(fromDecoder1); rcet.fromDecoder2(fromDecoder2);
//  rcet.fromNetwork1(fromNetwork1); rcet.fromNetwork2(fromNetwork2);
//  rcet.toALU1(toALU1); rcet.toALU2(toALU2);
//
//  Word w1(1), w2(2), w3(3), w4(4), w5(5);
//  Data d;
//
//  fromNetwork1.write(w1);     // b0 = 1             b1 = empty
//
//  TIMESTEP;
//
//  fromNetwork1.write(w2);
//  fromNetwork2.write(w3);     // b0 = 1, 2          b1 = 3
//
//  TIMESTEP;
//
//  fromNetwork1.write(w4);
//  fromNetwork2.write(w5);     // b0 = 1, 2, 4       b1 = 3, 5
//  fromDecoder1.write(0);
//  fromDecoder2.write(1);      // b0 = 2, 4          b1 = 5
//
//  TIMESTEP;
//
//  d = toALU1.read();
//  EXPECT_EQ(1, d.getData());
//  d = toALU2.read();
//  EXPECT_EQ(3, d.getData());
//  fromDecoder1.write(0);
//  fromDecoder2.write(1);      // b0 = 4             b1 = empty
//
//  TIMESTEP;
//
//  d = toALU1.read();
//  EXPECT_EQ(2, d.getData());
//  d = toALU2.read();
//  EXPECT_EQ(5, d.getData());
//  fromDecoder1.write(0);
//  fromDecoder2.write(1);      // b0 = empty         b1 = empty
//
//  TIMESTEP;
//
//  d = toALU1.read();
//  EXPECT_EQ(4, d.getData());
//  d = toALU2.read();
//  EXPECT_EQ(0, d.getData());  // Reading from empty buffer (should stall?)
//
//  // What happens if the same channel is asked for at both outputs?
//  // Send same value to both, or send subsequent values?
//  // Is this even allowed?
//
//}
