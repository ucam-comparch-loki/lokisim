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

/* Tests that implementation is complete enough for a Tile to be created, even
 * if it is not yet functional. */
//TEST_F(TileTest, CanInstantiateTile) {
//  TIMESTEP;
//}

/* Tests that clusters and memories within a tile can communicate usefully. */
//TEST_F(TileTest, Communication) {
//
//  string cluster0 = "setfetchch.loki";
//  string cluster1 = "remotefetch.loki";
//  string memory = "fibonacci2.loki";
//
//  CodeLoader::loadCode(cluster0, tile, 0);
//  CodeLoader::loadCode(cluster1, tile, 1);
//  CodeLoader::loadCode(memory, tile, 12);
//
//  // Should set DEBUG to true (or set up tracing) so values can be seen
//  for(int i=0; i<50; i++) TIMESTEP;
//
//}

/* Tests that load and store operations work. Adds together two vectors of
 * length 10. The result should be 12, 14, 16, ..., 30, stored at memory
 * addresses 20, 21, 22, ..., 29. */
TEST_F(TileTest, VectorAdd) {

  string cluster0 = "vector_add2/0.loki";
  string cluster1 = "vector_add2/1.loki";
  string cluster2 = "vector_add2/2.loki";
  string memory0  = "vector_add2/12.loki";
  string memory1  = "vector_add2/13.data";

  CodeLoader::loadCode(cluster0, tile, 0);
  CodeLoader::loadCode(cluster1, tile, 1);
  CodeLoader::loadCode(cluster2, tile, 2);
  CodeLoader::loadCode(memory0, tile, 12);
  CodeLoader::loadCode(memory1, tile, 13);

  for(int i=0; i<100; i++) TIMESTEP;

}
