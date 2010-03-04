/*
 * MemoryMatTest.cpp
 *
 *  Created on: 4 Mar 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../TileComponents/MemoryMat.h"

TEST(MemoryMatTest, MemoryMatWorks) {

  MemoryMat m("memory", 1);

  sc_clock clock("clock", 1, SC_NS, 0.5);
  flag_signal<Word> in[NUM_CLUSTER_INPUTS];
  flag_signal<AddressedWord> out[NUM_CLUSTER_OUTPUTS];
  sc_signal<bool> fcIn[NUM_CLUSTER_INPUTS];
  sc_signal<bool> fcOut[NUM_CLUSTER_OUTPUTS];

  AddressedWord temp;

  m.clock(clock);
  for(int i=0; i<NUM_CLUSTER_INPUTS; i++) {
    m.in[i](in[i]);
    m.flowControlIn[i](fcIn[i]);
  }
  for(int i=0; i<NUM_CLUSTER_OUTPUTS; i++) {
    m.out[i](out[i]);
    m.flowControlOut[i](fcOut[i]);
  }

  Data d1(1), d2(2), d3(3);
  MemoryRequest write(0, 100, 3, false), read(0, 100, 3, true);

  fcIn[2].write(true);

  // Put some data in the memory
  in[2].write(write);
  TIMESTEP;
  in[2].write(d1);
  TIMESTEP;
  in[2].write(d2);
  TIMESTEP;
  in[2].write(d3);
  TIMESTEP;

  // Read the data from memory
  in[2].write(read);
  TIMESTEP;
  temp = out[2].read();
  EXPECT_EQ(d1, temp.getPayload());
  EXPECT_EQ(100, temp.getChannelID());
  TIMESTEP;
  temp = out[2].read();
  EXPECT_EQ(d2, temp.getPayload());
  EXPECT_EQ(100, temp.getChannelID());
  TIMESTEP;
  temp = out[2].read();
  EXPECT_EQ(d3, temp.getPayload());
  EXPECT_EQ(100, temp.getChannelID());

}
