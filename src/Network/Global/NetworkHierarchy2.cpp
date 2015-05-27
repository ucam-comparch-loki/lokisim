/*
 * NetworkHierarchy.cpp
 *
 *  Created on: 1 Jul 2014
 *      Author: db434
 */

#include "NetworkHierarchy2.h"

#include "../../Utility/Parameters.h"
#include "../Network.h"
#include "../Topologies/InstantCrossbar.h"

NetworkHierarchy2::NetworkHierarchy2(const sc_module_name &name,
                                     const unsigned int    sourcesPerTile,
                                     const unsigned int    destinationsPerTile,
                                     const unsigned int    buffersPerDestination) :
    Component(name),
    globalNetwork("global", 0, TOTAL_TILE_ROWS, TOTAL_TILE_COLUMNS, Network::TILE) {

  assert(sourcesPerTile > 0);
  assert(destinationsPerTile > 0);
  assert(buffersPerDestination > 0);

  initialise(sourcesPerTile, destinationsPerTile, buffersPerDestination);

}

NetworkHierarchy2::~NetworkHierarchy2() {
  for (unsigned int i=0; i<toRouter.size(); i++)
    delete toRouter[i];

  for (unsigned int i=0; i<fromRouter.size(); i++)
    delete fromRouter[i];
}

void NetworkHierarchy2::initialise(const unsigned int sourcesPerTile,
                                   const unsigned int destinationsPerTile,
                                   const unsigned int buffersPerDestination) {

  iData.init(NUM_TILES);
  oData.init(NUM_TILES);
  iReady.init(NUM_TILES);

  // For compute tiles, create the required number of ports. For halo tiles,
  // only have one of each.
  for (uint col = 0; col < TOTAL_TILE_COLUMNS; col++) {
    for (uint row = 0; row < TOTAL_TILE_ROWS; row++) {
      TileID tile(col, row);
      TileIndex tileIndex = tile.overallTileIndex();

      if (tile.isComputeTile()) {
        iData[tileIndex].init(sourcesPerTile);
        oData[tileIndex].init(destinationsPerTile);
        iReady[tileIndex].init(destinationsPerTile, buffersPerDestination);
      }
      else {
        iData[tileIndex].init(1);
        oData[tileIndex].init(1);
        iReady[tileIndex].init(1,1);
      }
    }
  }

  localToGlobalData.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  globalToLocalData.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  localToGlobalReady.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  globalToLocalReady.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);

  globalNetwork.clock(clock);

  for (uint col = 0; col < TOTAL_TILE_COLUMNS; col++) {
    for (uint row = 0; row < TOTAL_TILE_ROWS; row++) {
      TileID tile(col, row);

      createNetworkToRouter(tile);
      createNetworkFromRouter(tile);

      globalNetwork.iData2D(col,row)(localToGlobalData[col][row]);
      globalNetwork.oReady2D(col,row)(globalToLocalReady[col][row]);
      globalNetwork.oData2D(col,row)(globalToLocalData[col][row]);
    }
  }

}

void NetworkHierarchy2::createNetworkToRouter(TileID tile) {
  TileIndex index = tile.overallTileIndex();
  uint sourcesPerTile = iData[index].length();

  // TODO: don't bother creating a crossbar if sourcesPerTile == 1.
  // Just connect iData directly to localToGlobalData (or the router).
  InstantCrossbar* xbar = new InstantCrossbar(sc_gen_unique_name("to_router"), // name
                              ComponentID(tile,0), // ID
                              sourcesPerTile,      // inputs from components
                              1,                   // outputs to router
                              1,                   // outputs leading to each router
                              Network::NONE,       // hierarchy level not needed
                              1);                  // buffers behind each output

  xbar->clock(clock);
  xbar->oData[0](localToGlobalData[tile.x][tile.y]);
  xbar->iReady[0][0](globalToLocalReady[tile.x][tile.y]);
  xbar->iData(iData[index]);

  toRouter.push_back(xbar);
}

void NetworkHierarchy2::createNetworkFromRouter(TileID tile) {
  TileIndex index = tile.overallTileIndex();
  uint destinationsPerTile = oData[index].length();
  uint buffersPerDestination = iReady[index][0].length();

  InstantCrossbar* xbar = new InstantCrossbar(sc_gen_unique_name("from_router"), // name
                              ComponentID(tile,0), // ID
                              1,                   // inputs from router
                              destinationsPerTile, // outputs to components
                              1,                   // outputs leading to each component
                              Network::COMPONENT,  // route by component number
                              buffersPerDestination);// buffers behind each output

  xbar->clock(clock);
  xbar->iData[0](globalToLocalData[tile.x][tile.y]);
  xbar->oData(oData[index]);
  xbar->iReady(iReady[index]);

  fromRouter.push_back(xbar);
}
