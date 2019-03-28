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
      routers[col].push_back(new Router<Word>(name.str().c_str(), routerID, params));
    }
  }
}

void Mesh::wireUp(size2d_t tiles) {
  for (unsigned int col=0; col<tiles.width; col++) {
    for (unsigned int row=0; row<tiles.height; row++) {
      Router<Word>& router = routers[col][row];
      router.clock(clock);

      if (row > 0)
        router.outputs[NORTH](routers[col][row-1].inputs[SOUTH]);
      else {
        NetworkDeadEnd2<Word>* northEdge =
            new NetworkDeadEnd2<Word>(sc_gen_unique_name("north_edge"), TileID(col, row), "north");
        router.outputs[NORTH](*northEdge);
        edges.push_back(northEdge);
      }

      if (col > 0)
        router.outputs[EAST](routers[col-1][row].inputs[WEST]);
      else {
        NetworkDeadEnd2<Word>* eastEdge =
            new NetworkDeadEnd2<Word>(sc_gen_unique_name("east_edge"), TileID(col, row), "east");
        router.outputs[EAST](*eastEdge);
        edges.push_back(eastEdge);
      }

      if (row < tiles.height-1)
        router.outputs[SOUTH](routers[col][row+1].inputs[NORTH]);
      else {
        NetworkDeadEnd2<Word>* southEdge =
            new NetworkDeadEnd2<Word>(sc_gen_unique_name("south_edge"), TileID(col, row), "south");
        router.outputs[SOUTH](*southEdge);
        edges.push_back(southEdge);
      }

      if (col < tiles.width-1)
        router.outputs[WEST](routers[col+1][row].inputs[EAST]);
      else {
        NetworkDeadEnd2<Word>* westEdge =
            new NetworkDeadEnd2<Word>(sc_gen_unique_name("west_edge"), TileID(col, row), "west");
        router.outputs[WEST](*westEdge);
        edges.push_back(westEdge);
      }

      // TODO
      // Data heading to/from local tile
      router.iData(iData[col][row]);
      router.oData(oData[col][row]);
      router.oReady(oReady[col][row]);
      router.iReady(iReady[col][row]);
    }
  }
}

Mesh::Mesh(const sc_module_name& name,
           size2d_t size,
           const router_parameters_t& routerParams) :
    Network(name),
    clock("clock"),
    iData("iData", size.width, size.height),
    oData("oData", size.width, size.height),
    oReady("oReady", size.width, size.height),
    iReady("iReady", size.width, size.height) {

  makeRouters(size, routerParams);
  wireUp(size);

}
