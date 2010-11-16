/*
 * Router.cpp
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#include "Router.h"
#include "Arbiters/RoundRobinArbiter.h"

ChannelIndex Router::computeOutput(ChannelIndex source,
                                   ChannelID destination) const {
  int destTile   = destination / inputsPerTile;
  int destColumn = destTile % NUM_TILE_COLUMNS;
  int destRow    = destTile / NUM_TILE_COLUMNS;

  // Use basic XY routing for now: move along X dimension until we reach the
  // correct column, then move along Y dimension.
  if(destColumn < column) return WEST;
  else if(destColumn > column) return EAST;
  else if(destRow < row) return NORTH;
  else if(destRow > row) return SOUTH;
  else return LOCAL;
}

Router::Router(sc_module_name name, ComponentID ID, int inputsPerTile,
               Arbiter* arbiter) :
    RoutingComponent(name, ID, 5, 5, ROUTER_BUFFER_SIZE),
    inputsPerTile(inputsPerTile) {

  // Use our ID to determine where we are on the network.
  column = id % NUM_TILE_COLUMNS;
  row    = id / NUM_TILE_COLUMNS;

  arbiter_ = arbiter;

}

Router::~Router() {

}
