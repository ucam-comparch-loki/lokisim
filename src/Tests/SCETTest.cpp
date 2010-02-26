/*
 * SCETTest.cpp
 *
 *  Created on: 8 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../TileComponents/Pipeline/Write/SendChannelEndTable.h"

//TEST(SCETTest, SCETWorks) {
//
//  SendChannelEndTable scet("scet", 1);
//
//  sc_signal<Word> input;
//  sc_signal<short> channel;
//  sc_signal<Array<AddressedWord> > output;
//  sc_signal<Array<bool> > flowControl;
//
//  scet.input(input);
//  scet.remoteChannel(channel);
//  scet.output(output);
//  scet.flowControl(flowControl);
//
//  Word w1(1), w2(2);
//  Array<bool> control = *(new Array<bool>(2));
//  bool t = true, f = false;
//  control.put(0, f);
//  control.put(1, f);
//  Array<AddressedWord> temp;
//
//  input.write(w1);
//  channel.write(0);
//  flowControl.write(control);
//
//  TIMESTEP;
//
//  input.write(w2);
//  channel.write(1);
//
//  TIMESTEP;
//
//  control.put(1, t);
//  flowControl.write(control);
//
//  TIMESTEP;
//
//  temp = output.read();
//  EXPECT_EQ(AddressedWord(), temp.get(0));
//  EXPECT_EQ(w2, temp.get(1).getPayload());
//  control.put(0, t);
//  flowControl.write(control);
//
//  TIMESTEP;
//
//  temp = output.read();
//  EXPECT_EQ(w1, temp.get(0).getPayload());
//  EXPECT_EQ(w2, temp.get(1).getPayload());
//
//}
