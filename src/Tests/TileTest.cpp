/*
 * TileTest.cpp
 *
 *  Created on: 23 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Tile.h"
#include "../Utility/CodeLoader.h"

class TileTest : public ::testing::Test {
protected:

  Tile t;

  sc_clock clock;
//  sc_signal<AddressedWord> *inNorth,  *inEast,  *inSouth,  *inWest,
//                           *outNorth, *outEast, *outSouth, *outWest;

//  sc_core::sc_trace_file *trace;

  TileTest() :
      t("tile", 0),
      clock("clock", 1, SC_NS, 0.5) {

//    inNorth  = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    inEast   = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    inSouth  = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    inWest   = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    outNorth = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    outEast  = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    outSouth = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    outWest  = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];

    t.clock(clock);

//    for(int i=0; i<NUM_CHANNELS_BETWEEN_TILES; i++) {
//      t.inNorth[i](inNorth[i]);
//      t.inEast[i](inEast[i]);
//      t.inSouth[i](inSouth[i]);
//      t.inWest[i](inWest[i]);
//      t.outNorth[i](outNorth[i]);
//      t.outEast[i](outEast[i]);
//      t.outSouth[i](outSouth[i]);
//      t.outWest[i](outWest[i]);
//    }

  }

  virtual void SetUp() {
//    trace = sc_core::sc_create_vcd_trace_file("TileTest");
//    trace->set_time_unit(1.0, SC_NS);
//
//    for(int i=0; i<NUM_CLUSTER_OUTPUTS; i++) {
//      flowControlIn[i].write(false);
//
//      std::string name("output");
//      sc_core::sc_trace(trace, out[i], name);
//    }
  }

  virtual void TearDown() {
//    TIMESTEP;
//    sc_core::sc_close_vcd_trace_file(trace);
  }

};

//TEST_F(TileTest, CanInstantiateTile) {
//  TIMESTEP;
//}

TEST_F(TileTest, Communication) {

  char* cluster0 = "/home/db434/Documents/Simulator/Test Code/setfetchch.loki";
  char* cluster1 = "/home/db434/Documents/Simulator/Test Code/remotefetch.loki";
  char* memory = "/home/db434/Documents/Simulator/Test Code/fibonacci2.loki";

  CodeLoader::loadCode(cluster0, t, 0);
  CodeLoader::loadCode(cluster1, t, 1);
  CodeLoader::loadCode(memory, t, 12);

  // Should set DEBUG to true (or set up tracing) so values can be seen
  ASSERT_NO_THROW(sc_start(50.0, SC_NS));

}
