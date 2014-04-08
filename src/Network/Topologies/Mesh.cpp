/*
 * Mesh.cpp
 *
 *  Created on: 29 Jun 2011
 *      Author: db434
 */

#include "Mesh.h"
#include "../NetworkHierarchy.h"
#include "../Router.h"

void Mesh::makeRouters() {
  int tile = 0;
  for(unsigned int row=0; row<numRows; row++) {
    for(unsigned int col=0; col<numColumns; col++) {
      ComponentID routerID(tile, COMPONENTS_PER_TILE);
      local_net_t* network = ((NetworkHierarchy*)(get_parent()))->getLocalNetwork(routerID);
      routers[col][row] = new Router(sc_gen_unique_name("router"), routerID, carriesCredits, network);
      tile++;
    }
  }
}

void Mesh::makeWires() {
  // Note that there is one more column and one more row of wires than there
  // are routers. This is because there are wires on each side of each router.
  dataSigNS.init(numColumns+1, numRows+1);
  dataSigSN.init(numColumns+1, numRows+1);
  dataSigEW.init(numColumns+1, numRows+1);
  dataSigWE.init(numColumns+1, numRows+1);
  readySigNS.init(numColumns+1, numRows+1);
  readySigSN.init(numColumns+1, numRows+1);
  readySigEW.init(numColumns+1, numRows+1);
  readySigWE.init(numColumns+1, numRows+1);
}

void Mesh::wireUp() {
  for(unsigned int col=0; col<numColumns; col++) {
    for(unsigned int row=0; row<numRows; row++) {
      Router& router = *routers[col][row];
      router.clock(clock);

      // Data heading north-south
      router.dataIn[Router::NORTH](dataSigNS[col][row]);
      router.dataOut[Router::SOUTH](dataSigNS[col][row+1]);
      router.readyIn[Router::NORTH](readySigNS[col][row]);
      router.readyOut[Router::SOUTH](readySigNS[col][row+1]);

      // Data heading east-west
      router.dataIn[Router::EAST](dataSigEW[col+1][row]);
      router.dataOut[Router::WEST](dataSigEW[col][row]);
      router.readyIn[Router::EAST](readySigEW[col+1][row]);
      router.readyOut[Router::WEST](readySigEW[col][row]);

      // Data heading south-north
      router.dataIn[Router::SOUTH](dataSigSN[col][row+1]);
      router.dataOut[Router::NORTH](dataSigSN[col][row]);
      router.readyIn[Router::SOUTH](readySigSN[col][row+1]);
      router.readyOut[Router::NORTH](readySigSN[col][row]);

      // Data heading west-east
      router.dataIn[Router::WEST](dataSigWE[col][row]);
      router.dataOut[Router::EAST](dataSigWE[col+1][row]);
      router.readyIn[Router::WEST](readySigWE[col][row]);
      router.readyOut[Router::EAST](readySigWE[col+1][row]);

      // Data heading to/from local tile
      int tile = router.id.getTile();
      router.dataIn[Router::LOCAL](dataIn[tile]);
      router.dataOut[Router::LOCAL](dataOut[tile]);
      router.readyOut[Router::LOCAL](readyOut[tile]);
    }
  }
}

Mesh::Mesh(const sc_module_name& name,
           ComponentID ID,
           int rows,
           int columns,
           bool carriesCredits,
           HierarchyLevel level) :
    Network(name, ID, rows*columns, rows*columns, level),
    routers(columns, std::vector<Router*>(rows)),
    numColumns(columns),
    numRows(rows),
    carriesCredits(carriesCredits) {

  // Can only handle inter-tile mesh networks at the moment.
  assert(level == Network::TILE);

  readyOut.init(rows*columns);

  makeRouters();
  makeWires();
  wireUp();

}

Mesh::~Mesh() {
  for (unsigned int i=0; i<routers.size(); i++)
    for (unsigned int j=0; j<routers[i].size(); j++)
      delete routers[i][j];
}
