/*
 * NetworkHierarchy.cpp
 *
 *  Created on: 1 Jul 2014
 *      Author: db434
 */

#include "NetworkHierarchy2.h"

#include "../../Datatype/Identifier.h"
#include "../../Datatype/Word.h"
#include "../../Typedefs.h"
#include "../../Utility/Assert.h"
#include "../../Utility/LokiVector.h"
#include "../../Utility/LokiVector2D.h"
#include "../../Utility/LokiVector3D.h"
#include "../../Utility/Parameters.h"
#include "../Network.h"
#include "../Topologies/InstantCrossbar.h"
#include "../WormholeMultiplexer.h"

NetworkHierarchy2::NetworkHierarchy2(const sc_module_name &name,
                                     const unsigned int    sourcesPerTile,
                                     const unsigned int    destinationsPerTile,
                                     const unsigned int    buffersPerDestination) :
    LokiComponent(name),
    globalNetwork("global", 0, TOTAL_TILE_ROWS, TOTAL_TILE_COLUMNS, Network::TILE) {

  loki_assert(sourcesPerTile > 0);
  loki_assert(destinationsPerTile > 0);
  loki_assert(buffersPerDestination > 0);

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
    }
  }

  globalNetwork.iData(localToGlobalData);
  globalNetwork.oReady(globalToLocalReady);
  globalNetwork.oData(globalToLocalData);
  globalNetwork.iReady(localToGlobalReady);

}

void NetworkHierarchy2::createNetworkToRouter(TileID tile) {
  TileIndex index = tile.overallTileIndex();
  uint sourcesPerTile = iData[index].length();

  WormholeMultiplexer<Word>* mux = new WormholeMultiplexer<Word>(
      sc_gen_unique_name("to_router"), sourcesPerTile);
  mux->iData(iData[index]);
  mux->oData(localToGlobalData[tile.x][tile.y]);
  toRouter.push_back(mux);
}

void NetworkHierarchy2::createNetworkFromRouter(TileID tile) {
  TileIndex index = tile.overallTileIndex();
  uint destinationsPerTile = oData[index].length();
  uint buffersPerDestination = iReady[index][0].length();

  RouterDemultiplexer<Word>* demux = new RouterDemultiplexer<Word>(
      sc_gen_unique_name("from_router"), destinationsPerTile, buffersPerDestination);
  demux->iData(globalToLocalData[tile.x][tile.y]);
  demux->oData(oData[index]);
  demux->iReady(iReady[index]);
  demux->oReady(localToGlobalReady[tile.x][tile.y]);
  fromRouter.push_back(demux);
}
