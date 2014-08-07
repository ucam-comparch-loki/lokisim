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
                                     const unsigned int    components,
                                     const unsigned int    buffersPerComponent) :
    Component(name),
    globalNetwork("global", 0, NUM_TILE_ROWS, NUM_TILE_COLUMNS, Network::TILE) {

  initialise(components, buffersPerComponent);

}

NetworkHierarchy2::~NetworkHierarchy2() {
  for (unsigned int i=0; i<toRouter.size(); i++)
    delete toRouter[i];

  for (unsigned int i=0; i<fromRouter.size(); i++)
    delete fromRouter[i];
}

void NetworkHierarchy2::initialise(const unsigned int components,
                                   const unsigned int buffersPerComponent) {

  iData.init(components);
  oData.init(components);  // one per input PORT
  iReady.init(components, buffersPerComponent); // one per input BUFFER

  localToGlobalData.init(NUM_TILES);
  globalToLocalData.init(NUM_TILES);
  localToGlobalReady.init(NUM_TILES);
  globalToLocalReady.init(NUM_TILES);

  // Create local networks and wire them up.
  int sourceComponentCounter = 0;
  int destComponentCounter = 0;

  for (unsigned int tile=0; tile<NUM_TILES; tile++) {
    InstantCrossbar* xbar = new InstantCrossbar(sc_gen_unique_name("to_router"), // name
                                  ComponentID(tile,0), // ID
                                  components/NUM_TILES,// inputs from components
                                  1,                   // outputs to router
                                  1,                   // outputs leading to each router
                                  Network::NONE,       // hierarchy level not needed
                                  1);                  // buffers behind each output

    xbar->clock(clock);
    xbar->oData[0](localToGlobalData[tile]);
    xbar->iReady[0][0](globalToLocalReady[tile]);
    for (uint component=0; component<components/NUM_TILES; component++)
      xbar->iData[component](iData[sourceComponentCounter++]);

    globalNetwork.iData[tile](localToGlobalData[tile]);
    globalNetwork.oReady[tile](globalToLocalReady[tile]);

    toRouter.push_back(xbar);

    InstantCrossbar* xbar2 = new InstantCrossbar(sc_gen_unique_name("from_router"), // name
                                   ComponentID(tile,0), // ID
                                   1,                   // inputs from router
                                   components/NUM_TILES,// outputs to components
                                   1,                   // outputs leading to each component
                                   Network::COMPONENT,  // route by component number
                                   buffersPerComponent);// buffers behind each output

    xbar2->clock(clock);
    xbar2->iData[0](globalToLocalData[tile]);
    globalNetwork.oData[tile](globalToLocalData[tile]);

    for (uint component=0; component<components/NUM_TILES; component++) {
      xbar2->oData[component](oData[destComponentCounter]);
      for (uint buffer=0; buffer<buffersPerComponent; buffer++)
        xbar2->iReady[component][buffer](iReady[destComponentCounter][buffer]);
      destComponentCounter++;
    }

    fromRouter.push_back(xbar2);
  }

  globalNetwork.clock(clock);

}
