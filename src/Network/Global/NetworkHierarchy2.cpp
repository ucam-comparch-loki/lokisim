/*
 * NetworkHierarchy.cpp
 *
 *  Created on: 1 Jul 2014
 *      Author: db434
 */

#include "NetworkHierarchy2.h"
#include "../Network.h"
#include "../Topologies/InstantCrossbar.h"

NetworkHierarchy2::NetworkHierarchy2(const sc_module_name &name,
                                     const unsigned int    sourcesPerTile,
                                     const unsigned int    destinationsPerTile,
                                     const unsigned int    buffersPerDestination) :
    Component(name),
    globalNetwork("global", 0, COMPUTE_TILE_ROWS, COMPUTE_TILE_COLUMNS, Network::TILE) {

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

  iData.init(sourcesPerTile * NUM_COMPUTE_TILES);
  oData.init(destinationsPerTile * NUM_COMPUTE_TILES);
  iReady.init(destinationsPerTile * NUM_COMPUTE_TILES, buffersPerDestination);

  localToGlobalData.init(NUM_COMPUTE_TILES);
  globalToLocalData.init(NUM_COMPUTE_TILES);
  localToGlobalReady.init(NUM_COMPUTE_TILES);
  globalToLocalReady.init(NUM_COMPUTE_TILES);

  // Create local networks and wire them up.
  int sourceComponentCounter = 0;
  int destComponentCounter = 0;

  uint tile = 0;
  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
    	// TODO: don't bother creating a crossbar if sourcesPerTile == 1.
    	// Just connect iData directly to localToGlobalData.
    	InstantCrossbar* xbar = new InstantCrossbar(sc_gen_unique_name("to_router"), // name
                                  ComponentID(col,row,0), // ID
                                  sourcesPerTile,      // inputs from components
                                  1,                   // outputs to router
                                  1,                   // outputs leading to each router
                                  Network::NONE,       // hierarchy level not needed
                                  1);                  // buffers behind each output

    	xbar->clock(clock);
    	xbar->oData[0](localToGlobalData[tile]);
    	xbar->iReady[0][0](globalToLocalReady[tile]);
    	for (uint component=0; component<sourcesPerTile; component++)
      	xbar->iData[component](iData[sourceComponentCounter++]);

    	globalNetwork.iData[tile](localToGlobalData[tile]);
    	globalNetwork.oReady[tile](globalToLocalReady[tile]);

    	toRouter.push_back(xbar);

    	InstantCrossbar* xbar2 = new InstantCrossbar(sc_gen_unique_name("from_router"), // name
                                   ComponentID(col,row,0), // ID
                                   1,                   // inputs from router
                                   destinationsPerTile, // outputs to components
                                   1,                   // outputs leading to each component
                                   Network::COMPONENT,  // route by component number
                                   buffersPerDestination);// buffers behind each output

    	xbar2->clock(clock);
    	xbar2->iData[0](globalToLocalData[tile]);
    	globalNetwork.oData[tile](globalToLocalData[tile]);

    	for (uint component=0; component<destinationsPerTile; component++) {
      	xbar2->oData[component](oData[destComponentCounter]);
      	for (uint buffer=0; buffer<buffersPerDestination; buffer++)
        	xbar2->iReady[component][buffer](iReady[destComponentCounter][buffer]);
      	destComponentCounter++;
    	}

    	fromRouter.push_back(xbar2);
    	
    	tile++;
   	}
  }

  globalNetwork.clock(clock);

}
