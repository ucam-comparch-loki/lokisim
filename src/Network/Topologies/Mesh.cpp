/*
 * Mesh.cpp
 *
 *  Created on: 29 Jun 2011
 *      Author: db434
 */

#include "Mesh.h"
#include "../Router.h"
#include "../../Utility/Assert.h"

void Mesh::makeRouters() {
  for (unsigned int row=0; row<numRows; row++) {
    for (unsigned int col=0; col<numColumns; col++) {
      ComponentID routerID(col, row, COMPONENTS_PER_TILE);
      std::stringstream name;
      name << "router_" << routerID.tile.getNameString();
      routers[col][row] = new Router(name.str().c_str(), routerID);
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
  for (unsigned int col=0; col<numColumns; col++) {
    for (unsigned int row=0; row<numRows; row++) {
      Router& router = *routers[col][row];
      router.clock(clock);

      // Data heading north-south
      router.iData[NORTH](dataSigNS[col][row]);
      router.oData[SOUTH](dataSigNS[col][row+1]);
      router.iReady[NORTH](readySigNS[col][row]);
      router.oReady[SOUTH](readySigNS[col][row+1]);

      // Data heading east-west
      router.iData[EAST](dataSigEW[col+1][row]);
      router.oData[WEST](dataSigEW[col][row]);
      router.iReady[EAST](readySigEW[col+1][row]);
      router.oReady[WEST](readySigEW[col][row]);

      // Data heading south-north
      router.iData[SOUTH](dataSigSN[col][row+1]);
      router.oData[NORTH](dataSigSN[col][row]);
      router.iReady[SOUTH](readySigSN[col][row+1]);
      router.oReady[NORTH](readySigSN[col][row]);

      // Data heading west-east
      router.iData[WEST](dataSigWE[col][row]);
      router.oData[EAST](dataSigWE[col+1][row]);
      router.iReady[WEST](readySigWE[col][row]);
      router.oReady[EAST](readySigWE[col+1][row]);

      // Data heading to/from local tile
      router.iData[LOCAL](iData[col][row]);
      router.oData[LOCAL](oData[col][row]);
      router.oReady[LOCAL](oReady[col][row]);
      router.iReady[LOCAL](iReady[col][row]);
    }
  }

  // Tie off the wires at the edges of the network so we can get useful debug
  // information if any data is sent on them.
  for (uint row=0; row<numRows; row++) {
    NetworkDeadEnd<NetworkData>* westEdge =
        new NetworkDeadEnd<NetworkData>(sc_gen_unique_name("west_edge"), ComponentID(0, row, 0), WEST);
    westEdge->iData(dataSigEW[0][row]);
    westEdge->oData(dataSigWE[0][row]);
    westEdge->iReady(readySigEW[0][row]);
    westEdge->oReady(readySigWE[0][row]);
    edges.push_back(westEdge);

    NetworkDeadEnd<NetworkData>* eastEdge =
        new NetworkDeadEnd<NetworkData>(sc_gen_unique_name("east_edge"), ComponentID(numColumns-1, row, 0), EAST);
    eastEdge->iData(dataSigWE[numColumns][row]);
    eastEdge->oData(dataSigEW[numColumns][row]);
    eastEdge->iReady(readySigWE[numColumns][row]);
    eastEdge->oReady(readySigEW[numColumns][row]);
    edges.push_back(eastEdge);
  }

  for (uint col=0; col<numColumns; col++) {
    NetworkDeadEnd<NetworkData>* northEdge =
        new NetworkDeadEnd<NetworkData>(sc_gen_unique_name("north_edge"), ComponentID(col, 0, 0), NORTH);
    northEdge->iData(dataSigSN[col][0]);
    northEdge->oData(dataSigNS[col][0]);
    northEdge->iReady(readySigSN[col][0]);
    northEdge->oReady(readySigNS[col][0]);
    edges.push_back(northEdge);

    NetworkDeadEnd<NetworkData>* southEdge =
        new NetworkDeadEnd<NetworkData>(sc_gen_unique_name("south_edge"), ComponentID(col, numRows-1, 0), SOUTH);
    southEdge->iData(dataSigNS[col][numRows]);
    southEdge->oData(dataSigSN[col][numRows]);
    southEdge->iReady(readySigNS[col][numRows]);
    southEdge->oReady(readySigSN[col][numRows]);
    edges.push_back(southEdge);
  }
}

Mesh::Mesh(const sc_module_name& name,
           ComponentID ID,
           int rows,
           int columns,
           HierarchyLevel level) :
    Network(name, ID, rows*columns, rows*columns, level),
    routers(columns, std::vector<Router*>(rows)),
    numColumns(columns),
    numRows(rows) {

  // Can only handle inter-tile mesh networks at the moment.
  loki_assert(level == Network::TILE);

  iData.init(columns, rows);
  oData.init(columns, rows);
  oReady.init(columns, rows);
  iReady.init(columns, rows);

  makeRouters();
  makeWires();
  wireUp();

}

Mesh::~Mesh() {
  for (unsigned int i=0; i<routers.size(); i++)
    for (unsigned int j=0; j<routers[i].size(); j++)
      delete routers[i][j];

  for (unsigned int i=0; i<edges.size(); i++)
    delete edges[i];
}
