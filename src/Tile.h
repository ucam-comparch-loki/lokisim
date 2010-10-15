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
#include "flag_signal.h"
#include "Network/InterclusterNetwork.h"

using std::vector;

class WrappedTileComponent;

class Tile : public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>  clock;
//  sc_in<AddressedWord>  *inNorth,  *inEast,  *inSouth,  *inWest;
//  sc_out<AddressedWord> *outNorth, *outEast, *outSouth, *outWest;

  sc_out<bool> idle;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Tile);
  Tile(sc_module_name name, int ID);
  virtual ~Tile();

//==============================//
// Methods
//==============================//

public:

  virtual double area()   const;
  virtual double energy() const;

  bool isIdle() const;

  // Store the given instructions or data into the component at the given index.
  void storeData(vector<Word>& data, int componentNumber);

  void print(int component, int start, int end) const;
  Word getMemVal(int component, int addr) const;
  int  getRegVal(int component, int reg) const;
  int  getInstIndex(int component) const;
  bool getPredReg(int component) const;

  // Static functions for connecting Tiles together
  static void connectLeftRight(const Tile& left, const Tile& right);
  static void connectTopBottom(const Tile& top, const Tile& bottom);

private:

  void updateIdle();

//==============================//
// Components
//==============================//

private:

  vector<WrappedTileComponent*> contents;  // Clusters and memories of this tile
  InterclusterNetwork           network;

//==============================//
// Signals (wires)
//==============================//

private:

  flag_signal<AddressedWord> *responsesToCluster, *requestsToCluster; // arrays
  flag_signal<AddressedWord> *dataToCluster;                          // array
  flag_signal<AddressedWord> *responsesFromCluster;                   // array
  sc_signal<bool>            *idleSig;                                // array

//==============================//
// Local state
//==============================//

private:

  bool idleVal;

};

#endif /* TILE_H_ */
