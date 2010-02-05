/*
 * Tile.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Tile.h"

/* Connect two horizontally-adjacent Tiles together. */
void Tile::connectLeftRight(Tile& left, Tile& right) {
  for(int i=0; i<NUM_CHANNELS_BETWEEN_TILES; i++) {
    sc_signal<Array<AddressedWord> > leftToRight; // TODO: Don't allocate on stack
    left.outEast(leftToRight);
    right.inWest(leftToRight);

    sc_signal<Array<AddressedWord> > rightToLeft;
    left.inEast(rightToLeft);
    right.outWest(rightToLeft);
  }
}

/* Connect two vertically-adjacent Tiles together. */
void Tile::connectTopBottom(Tile& top, Tile& bottom) {
  for(int i=0; i<NUM_CHANNELS_BETWEEN_TILES; i++) {
    sc_signal<Array<AddressedWord> > topToBottom;
    top.outSouth(topToBottom);
    bottom.inNorth(topToBottom);

    sc_signal<Array<AddressedWord> > bottomToTop;
    top.inSouth(bottomToTop);
    bottom.outNorth(bottomToTop);
  }
}

/* Constructors and destructor. */
Tile::Tile() : Component("tile") { // Give a default name
  Tile("tile");
}

Tile::Tile(sc_core::sc_module_name name) : Component(name) {

  static int ID = 0;

  // Initialise the Clusters of this Tile
  for(int i=0; i<CLUSTERS_PER_TILE; i++) {
    // Does the Cluster need to be "new"?
    contents.push_back(Cluster("cluster", ID*(CLUSTERS_PER_TILE+MEMS_PER_TILE) + i));
  }

  // Initialise the memories of this Tile
  for(int i=CLUSTERS_PER_TILE; i<CLUSTERS_PER_TILE+MEMS_PER_TILE; i++) {
    // Does the memory need to be "new"?
    contents.push_back(MemoryMat("memory", ID*(CLUSTERS_PER_TILE+MEMS_PER_TILE) + i));
  }

  // Initialise the ports of this Tile (NUM_CHANNELS_BETWEEN_TILES in each direction)
//  for(int j=0; j<NUM_CHANNELS_BETWEEN_TILES; j++) {
//    // Do the ports need to be "new"?
//    sc_in<AddressedWord> in1, in2, in3, in4;
//    inNorth.push_back(in1);
//    inEast.push_back(in2);
//    inSouth.push_back(in3);
//    inWest.push_back(in4);
//
//    sc_out<AddressedWord> out1, out2, out3, out4;
//    outNorth.push_back(out1);
//    outEast.push_back(out2);
//    outSouth.push_back(out3);
//    outWest.push_back(out4);
//  }

  // TODO: create interconnect and connect things up

  ID++;

}

Tile::Tile(const Tile& other) : Component(other) {
  contents = other.contents;
}

Tile::~Tile() {

}
