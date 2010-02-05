/*
 * MultiplexorTest.cpp
 *
 * Class responsible for testing Multiplexors. Currently only tests one
 * particular multiplexor, but they all work the same so this should be enough?
 *
 * All tests but one are commented out until I can get Google Test and SystemC
 * working together nicely.
 *
 *  Created on: 20 Jan 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include <systemc.h>
#include "Test.h"
#include "../Multiplexor/Multiplexor2.h"
#include "../Multiplexor/Multiplexor3.h"
#include "../Datatype/Data.h"


//TEST(MultiplexorTest, Multiplexor2WorksCorrectly) {
//
//  Multiplexor2<int> m("mult");
//
//  sc_signal<int> in1, in2, out;
//  sc_signal<short> select;
//
//  m.in1(in1); m.in2(in2); m.result(out); m.select(select);
//
//  in1.write(1); in2.write(2);
//
//  select.write(0);
//  TIMESTEP;
//  EXPECT_EQ(1, out.read());
//
//  select.write(1);
//  TIMESTEP;
//  EXPECT_EQ(2, out.read());
//
//  // Test an invalid input? How do we EXPECT_FAILURE?
//
//}

/* Tests that multiplexors give correct outputs */
//TEST(MultiplexorTest, Multiplexor3WorksCorrectly) {
//
//  Multiplexor3<int> m("mult");
//
//  sc_signal<int> in1, in2, in3, out;
//  sc_signal<short> select;
//
//  m.in1(in1); m.in2(in2); m.in3(in3); m.result(out); m.select(select);
//
//  in1.write(1); in2.write(2); in3.write(3);
//
//  select.write(0);
//  TIMESTEP;
//  EXPECT_EQ(1, out.read());
//
//  select.write(1);
//  TIMESTEP;
//  EXPECT_EQ(2, out.read());
//
//  select.write(2);
//  TIMESTEP;
//  EXPECT_EQ(3, out.read());
//
//  // Test an invalid input? How do we EXPECT_FAILURE?
//
//}

//TEST(MultiplexorTest, MultiplexorsHandleOddTypes) {
//
//  Multiplexor2<Data> m("mult");
//
//  sc_signal<Data> in1, in2, out;
//  sc_signal<short> select;
//
//  m.in1(in1); m.in2(in2); m.result(out); m.select(select);
//
//  in1.write(Data(1)); in2.write(Data(2));
//
//  select.write(0);
//  TIMESTEP;
//  Data result = out.read();
//  EXPECT_EQ(1, result.getData());
//
//  select.write(1);
//  TIMESTEP;
//  result = out.read();
//  EXPECT_EQ(2, result.getData());
//
//  // Test an invalid input? How do we EXPECT_FAILURE?
//
//}
