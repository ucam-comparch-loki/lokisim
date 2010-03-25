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

  Tile tile;

  sc_clock clock;
//  sc_signal<AddressedWord> *inNorth,  *inEast,  *inSouth,  *inWest,
//                           *outNorth, *outEast, *outSouth, *outWest;

//  sc_core::sc_trace_file *trace;

  TileTest() :
      tile("tile", 0),
      clock("clock", 1, SC_NS, 0.5) {

//    inNorth  = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    inEast   = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    inSouth  = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    inWest   = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    outNorth = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    outEast  = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    outSouth = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//    outWest  = new sc_signal<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];

    tile.clock(clock);

//    for(int i=0; i<NUM_CHANNELS_BETWEEN_TILES; i++) {
//      tile.inNorth[i](inNorth[i]);
//      tile.inEast[i](inEast[i]);
//      tile.inSouth[i](inSouth[i]);
//      tile.inWest[i](inWest[i]);
//      tile.outNorth[i](outNorth[i]);
//      tile.outEast[i](outEast[i]);
//      tile.outSouth[i](outSouth[i]);
//      tile.outWest[i](outWest[i]);
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

  string cluster0 = "setfetchch.loki";
  string cluster1 = "remotefetch.loki";
  string memory = "fibonacci2.loki";

  CodeLoader::loadCode(cluster0, tile, 0);
  CodeLoader::loadCode(cluster1, tile, 1);
  CodeLoader::loadCode(memory, tile, 12);

  // Should set DEBUG to true (or set up tracing) so values can be seen
  for(int i=0; i<50; i++) TIMESTEP;

}
