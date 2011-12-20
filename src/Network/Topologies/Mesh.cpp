/*
 * Mesh.cpp
 *
 *  Created on: 29 Jun 2011
 *      Author: db434
 */

#include "Mesh.h"
#include "../Router.h"

void Mesh::makeRouters() {
  int tile = 0;
  for(unsigned int row=0; row<numRows; row++) {
    for(unsigned int col=0; col<numColumns; col++) {
      routers[col][row] = new Router(sc_gen_unique_name("router"),
                                     ComponentID(tile,0));
      tile++;
    }
  }
}

void Mesh::makeWires() {
  // Note that there is one more column and one more row of wires than there
  // are routers. This is because there are wires on each side of each router.

  dataSigNS  = new DataSignal*[numColumns+1];
  dataSigSN  = new DataSignal*[numColumns+1];
  dataSigEW  = new DataSignal*[numColumns+1];
  dataSigWE  = new DataSignal*[numColumns+1];

  for(unsigned int col=0; col<=numColumns; col++) {
    dataSigNS[col]  = new DataSignal[numRows+1];
    dataSigSN[col]  = new DataSignal[numRows+1];
    dataSigEW[col]  = new DataSignal[numRows+1];
    dataSigWE[col]  = new DataSignal[numRows+1];
  }
}

void Mesh::wireUp() {
  for(unsigned int col=0; col<numColumns; col++) {
    for(unsigned int row=0; row<numRows; row++) {
      Router& router = *routers[col][row];
      router.clock(clock);

      // Data heading north-south
      router.dataIn[Router::NORTH](dataSigNS[col][row]);
      router.dataOut[Router::SOUTH](dataSigNS[col][row+1]);

      // Data heading east-west
      router.dataIn[Router::EAST](dataSigEW[col+1][row]);
      router.dataOut[Router::WEST](dataSigEW[col][row]);

      // Data heading south-north
      router.dataIn[Router::SOUTH](dataSigSN[col][row+1]);
      router.dataOut[Router::NORTH](dataSigSN[col][row]);

      // Data heading west-east
      router.dataIn[Router::WEST](dataSigWE[col][row]);
      router.dataOut[Router::EAST](dataSigWE[col+1][row]);

      // Data heading to/from local tile
      int tile = router.id.getTile();
      router.dataIn[Router::LOCAL](dataIn[tile]);
      router.dataOut[Router::LOCAL](dataOut[tile]);
    }
  }
}

Mesh::Mesh(const sc_module_name& name,
           ComponentID ID,
           int rows,
           int columns,
           HierarchyLevel level,
           Dimension size) :
    Network(name, ID, rows*columns, rows*columns, level, size),
    routers(columns, std::vector<Router*>(rows)),
    numColumns(columns),
    numRows(rows) {

  // Can only handle inter-tile mesh networks at the moment.
  assert(level == Network::TILE);

  makeRouters();
  makeWires();
  wireUp();

}

Mesh::~Mesh() {
  for(unsigned int col=0; col<numColumns; col++) {
    for(unsigned int row=0; row<numRows; row++) delete routers[col][row];

    delete[] dataSigNS[col];    delete[] dataSigSN[col];
    delete[] dataSigEW[col];    delete[] dataSigWE[col];
  }

  delete[] dataSigNS;           delete[] dataSigSN;
  delete[] dataSigEW;           delete[] dataSigWE;
}
