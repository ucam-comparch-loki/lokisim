/*
 * Mesh.cpp
 *
 *  Created on: 29 Jun 2011
 *      Author: db434
 */

#include "Mesh.h"
#include "../Router.h"
#include "../../Utility/Assert.h"

void Mesh::makeRouters(size2d_t tiles, const router_parameters_t& params) {
  routers.init(tiles.width);

  for (unsigned int row=0; row<tiles.height; row++) {
    for (unsigned int col=0; col<tiles.width; col++) {
      TileID routerID(col, row);
      std::stringstream name;
      name << "router_" << routerID.getNameString();
      routers[col].push_back(new Router(name.str().c_str(), routerID, params));
    }
  }
}

void Mesh::makeWires(size2d_t tiles) {
  // Note that there is one more column and one more row of wires than there
  // are routers. This is because there are wires on each side of each router.
  dataSigNS.init("dataNS", tiles.width+1, tiles.height+1);
  dataSigSN.init("dataSN", tiles.width+1, tiles.height+1);
  dataSigEW.init("dataEW", tiles.width+1, tiles.height+1);
  dataSigWE.init("dataWE", tiles.width+1, tiles.height+1);
  readySigNS.init("readyNS", tiles.width+1, tiles.height+1);
  readySigSN.init("readySN", tiles.width+1, tiles.height+1);
  readySigEW.init("readyEW", tiles.width+1, tiles.height+1);
  readySigWE.init("readyWE", tiles.width+1, tiles.height+1);
}

void Mesh::wireUp(size2d_t tiles) {
  for (unsigned int col=0; col<tiles.width; col++) {
    for (unsigned int row=0; row<tiles.height; row++) {
      Router& router = routers[col][row];
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
  for (uint row=0; row<tiles.height; row++) {
    NetworkDeadEnd<NetworkData>* westEdge =
        new NetworkDeadEnd<NetworkData>(sc_gen_unique_name("west_edge"), TileID(0, row), WEST);
    westEdge->iData(dataSigEW[0][row]);
    westEdge->oData(dataSigWE[0][row]);
    westEdge->iReady(readySigEW[0][row]);
    westEdge->oReady(readySigWE[0][row]);
    edges.push_back(westEdge);

    NetworkDeadEnd<NetworkData>* eastEdge =
        new NetworkDeadEnd<NetworkData>(sc_gen_unique_name("east_edge"), TileID(tiles.width-1, row), EAST);
    eastEdge->iData(dataSigWE[tiles.width][row]);
    eastEdge->oData(dataSigEW[tiles.width][row]);
    eastEdge->iReady(readySigWE[tiles.width][row]);
    eastEdge->oReady(readySigEW[tiles.width][row]);
    edges.push_back(eastEdge);
  }

  for (uint col=0; col<tiles.width; col++) {
    NetworkDeadEnd<NetworkData>* northEdge =
        new NetworkDeadEnd<NetworkData>(sc_gen_unique_name("north_edge"), TileID(col, 0), NORTH);
    northEdge->iData(dataSigSN[col][0]);
    northEdge->oData(dataSigNS[col][0]);
    northEdge->iReady(readySigSN[col][0]);
    northEdge->oReady(readySigNS[col][0]);
    edges.push_back(northEdge);

    NetworkDeadEnd<NetworkData>* southEdge =
        new NetworkDeadEnd<NetworkData>(sc_gen_unique_name("south_edge"), TileID(col, tiles.height-1), SOUTH);
    southEdge->iData(dataSigNS[col][tiles.height]);
    southEdge->oData(dataSigSN[col][tiles.height]);
    southEdge->iReady(readySigNS[col][tiles.height]);
    southEdge->oReady(readySigSN[col][tiles.height]);
    edges.push_back(southEdge);
  }
}

Mesh::Mesh(const sc_module_name& name,
           size2d_t size,
           const router_parameters_t& routerParams) :
    Network(name, size.total(), size.total()),
    iData("iData", size.width, size.height),
    oData("oData", size.width, size.height),
    oReady("oReady", size.width, size.height),
    iReady("iReady", size.width, size.height) {

  makeRouters(size, routerParams);
  makeWires(size);
  wireUp(size);

}
