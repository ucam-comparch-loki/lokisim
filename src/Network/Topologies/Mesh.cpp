/*
 * Mesh.cpp
 *
 *  Created on: 29 Jun 2011
 *      Author: db434
 */

#include "Mesh.h"
#include "../Router.h"
#include "../../Utility/Assert.h"

Mesh::Mesh(const sc_module_name& name,
           size2d_t size,
           const router_parameters_t& routerParams) :
    LokiComponent(name),
    clock("clock"),
    inputs("inputs", size.width, size.height),
    outputs("outputs", size.width, size.height) {

  makeComponents(size, routerParams);
  wireUp(size);

}

void Mesh::makeComponents(size2d_t tiles, const router_parameters_t& params) {
  routers.init(tiles.width);

  for (unsigned int row=0; row<tiles.height; row++) {
    for (unsigned int col=0; col<tiles.width; col++) {
      TileID tile(col, row);
      std::stringstream routerName;
      routerName << "router_" << tile.getNameString();
      routers[col].push_back(new Router<Word>(routerName.str().c_str(), tile, params));
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
        NetworkDeadEnd<Word>* northEdge =
            new NetworkDeadEnd<Word>(sc_gen_unique_name("north_edge"), TileID(col, row), "north");
        router.outputs[NORTH](*northEdge);
        edges.push_back(northEdge);
      }

      if (col > 0)
        router.outputs[WEST](routers[col-1][row].inputs[EAST]);
      else {
        NetworkDeadEnd<Word>* westEdge =
            new NetworkDeadEnd<Word>(sc_gen_unique_name("west_edge"), TileID(col, row), "west");
        router.outputs[WEST](*westEdge);
        edges.push_back(westEdge);
      }

      if (row < tiles.height-1)
        router.outputs[SOUTH](routers[col][row+1].inputs[NORTH]);
      else {
        NetworkDeadEnd<Word>* southEdge =
            new NetworkDeadEnd<Word>(sc_gen_unique_name("south_edge"), TileID(col, row), "south");
        router.outputs[SOUTH](*southEdge);
        edges.push_back(southEdge);
      }

      if (col < tiles.width-1)
        router.outputs[EAST](routers[col+1][row].inputs[WEST]);
      else {
        NetworkDeadEnd<Word>* eastEdge =
            new NetworkDeadEnd<Word>(sc_gen_unique_name("east_edge"), TileID(col, row), "east");
        router.outputs[EAST](*eastEdge);
        edges.push_back(eastEdge);
      }

      // Data heading to/from local tile
      router.outputs[LOCAL](outputs[col][row]);
      inputs[col][row](router.inputs[LOCAL]);
    }
  }
}
