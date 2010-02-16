/*
 * TestClassTest.cpp
 *
 *  Created on: 16 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "TestClass.h"

//TEST(TestClassTest, SignalArraysWork) {
//
//  TestClass tc("test", 1, 4);
//
//  sc_buffer<bool> in[4], out[4];
//
//  for(int i=0; i<4; i++) {
//    tc.inputs[i](in[i]);
//    tc.outputs[i](out[i]);
//  }
//
//  in[0].write(true);
//  in[1].write(false);
//  in[2].write(false);
//  in[3].write(false);
//
//  TIMESTEP;
//
//  ASSERT_EQ(true, out[0].read());
//  ASSERT_EQ(false, out[1].read());
//  ASSERT_EQ(false, out[2].read());
//  ASSERT_EQ(false, out[3].read());
//
//  in[3].write(true);
//
//  TIMESTEP;
//
//  ASSERT_EQ(true, out[3].read());
//
//}
