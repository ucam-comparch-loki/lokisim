/*
 * Tile.h
 *
 * A class encapsulating a local group of Clusters and memories (collectively
 * known as TileComponents) to reduce the amount of global communication. Each
 * Tile is connected to its neighbours by a group of input and output ports at
 * each edge. It is up to the Tile to decide how routing is implemented.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef TILE_H_
#define TILE_H_

#include "Component.h"
#include "Cluster.h"
#include "Memory/MemoryMat.h"
#include "Datatype/AddressedWord.h"
#include "Datatype/Array.h"

using std::vector;

class Tile : public Component {

/* Components */
  // The clusters and memories of this tile.
  vector<TileComponent> contents;

  // Interconnect(s)

/* Ports */
  sc_in<Array<AddressedWord> > inNorth, inEast, inSouth, inWest;
  sc_out<Array<AddressedWord> > outNorth, outEast, outSouth, outWest;


public:
/* Methods */
  // Static functions for connecting Tiles together
  static void connectLeftRight(Tile& left, Tile& right);
  static void connectTopBottom(Tile& top, Tile& bottom);

/* Constructors and destructors */
  Tile();
  Tile(sc_core::sc_module_name name);
  Tile(const Tile& other);
  virtual ~Tile();
};

#endif /* TILE_H_ */
