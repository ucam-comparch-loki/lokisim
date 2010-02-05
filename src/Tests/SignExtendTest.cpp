/*
 * SignExtendTest.cpp
 *
 *  Created on: 4 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Pipeline/Decode/SignExtend.h"

//TEST(SignExtendTest, SignExtendWorks) {
//
//  SignExtend se("signextend", 1);
//
//  sc_signal<Data> in, out;
//  se.input(in);
//  se.output(out);
//
//  Data d1(1), d2(-456), d3(2079573849);
//  Data temp;
//
//  in.write(d1);
//
//  TIMESTEP;
//
//  in.write(d2);
//  temp = out.read();
//  EXPECT_EQ(d1.getData(), temp.getData());
//
//  TIMESTEP;
//
//  in.write(d3);
//  temp = out.read();
//  EXPECT_EQ(d2.getData(), (int)(temp.getData()));
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(d3.getData(), (int)(temp.getData()));
//
//}
