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
  for(int col=0; col<numColumns; col++) {
    for(int row=0; row<numRows; row++) {
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
  validSigNS = new ReadySignal*[numColumns+1];
  validSigSN = new ReadySignal*[numColumns+1];
  validSigEW = new ReadySignal*[numColumns+1];
  validSigWE = new ReadySignal*[numColumns+1];
  ackSigNS   = new ReadySignal*[numColumns+1];
  ackSigSN   = new ReadySignal*[numColumns+1];
  ackSigEW   = new ReadySignal*[numColumns+1];
  ackSigWE   = new ReadySignal*[numColumns+1];

  for(int col=0; col<=numColumns; col++) {
    dataSigNS[col]  = new DataSignal[numRows+1];
    dataSigSN[col]  = new DataSignal[numRows+1];
    dataSigEW[col]  = new DataSignal[numRows+1];
    dataSigWE[col]  = new DataSignal[numRows+1];
    validSigNS[col] = new ReadySignal[numRows+1];
    validSigSN[col] = new ReadySignal[numRows+1];
    validSigEW[col] = new ReadySignal[numRows+1];
    validSigWE[col] = new ReadySignal[numRows+1];
    ackSigNS[col]   = new ReadySignal[numRows+1];
    ackSigSN[col]   = new ReadySignal[numRows+1];
    ackSigEW[col]   = new ReadySignal[numRows+1];
    ackSigWE[col]   = new ReadySignal[numRows+1];
  }
}

void Mesh::wireUp() {
  for(int col=0; col<numColumns; col++) {
    for(int row=0; row<numRows; row++) {
      Router& router = *routers[col][row];
      router.clock(clock);

      // Data heading north-south
      router.dataIn[Router::NORTH](dataSigNS[col][row]);
      router.validDataIn[Router::NORTH](validSigNS[col][row]);
      router.ackDataIn[Router::NORTH](ackSigNS[col][row]);

      router.dataOut[Router::SOUTH](dataSigNS[col][row+1]);
      router.validDataOut[Router::SOUTH](validSigNS[col][row+1]);
      router.ackDataOut[Router::SOUTH](ackSigNS[col][row+1]);

      // Data heading east-west
      router.dataIn[Router::EAST](dataSigEW[col][row]);
      router.validDataIn[Router::EAST](validSigEW[col][row]);
      router.ackDataIn[Router::EAST](ackSigEW[col][row]);

      router.dataOut[Router::WEST](dataSigEW[col+1][row]);
      router.validDataOut[Router::WEST](validSigEW[col+1][row]);
      router.ackDataOut[Router::WEST](ackSigEW[col+1][row]);

      // Data heading south-north
      router.dataIn[Router::SOUTH](dataSigSN[col][row+1]);
      router.validDataIn[Router::SOUTH](validSigSN[col][row+1]);
      router.ackDataIn[Router::SOUTH](ackSigSN[col][row+1]);

      router.dataOut[Router::NORTH](dataSigSN[col][row]);
      router.validDataOut[Router::NORTH](validSigSN[col][row]);
      router.ackDataOut[Router::NORTH](ackSigSN[col][row]);

      // Data heading west-east
      router.dataIn[Router::WEST](dataSigWE[col+1][row]);
      router.validDataIn[Router::WEST](validSigWE[col+1][row]);
      router.ackDataIn[Router::WEST](ackSigWE[col+1][row]);

      router.dataOut[Router::EAST](dataSigWE[col][row]);
      router.validDataOut[Router::EAST](validSigWE[col][row]);
      router.ackDataOut[Router::EAST](ackSigWE[col][row]);

      // Data heading to/from local tile
      int tile = router.id.getTile();
      router.dataIn[Router::LOCAL](dataIn[tile]);
      router.validDataIn[Router::LOCAL](validDataIn[tile]);
      router.ackDataIn[Router::LOCAL](ackDataIn[tile]);

      router.dataOut[Router::LOCAL](dataOut[tile]);
      router.validDataOut[Router::LOCAL](validDataOut[tile]);
      router.ackDataOut[Router::LOCAL](ackDataOut[tile]);
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
    routers(columns, std::vector<Router*>(rows)){

  // Can only handle inter-tile mesh networks at the moment.
  assert(level == Network::TILE);

  numRows = rows;
  numColumns = columns;

  makeRouters();
  makeWires();
  wireUp();

}

Mesh::~Mesh() {
  for(int col=0; col<numColumns; col++) {
    for(int row=0; row<numRows; row++) delete routers[col][row];

    delete[] dataSigNS[col]; delete[] validSigNS[col]; delete[] ackSigNS[col];
    delete[] dataSigSN[col]; delete[] validSigSN[col]; delete[] ackSigSN[col];
    delete[] dataSigEW[col]; delete[] validSigEW[col]; delete[] ackSigEW[col];
    delete[] dataSigWE[col]; delete[] validSigWE[col]; delete[] ackSigWE[col];
  }

  delete[] dataSigNS; delete[] validSigNS; delete[] ackSigNS;
  delete[] dataSigSN; delete[] validSigSN; delete[] ackSigSN;
  delete[] dataSigEW; delete[] validSigEW; delete[] ackSigEW;
  delete[] dataSigWE; delete[] validSigWE; delete[] ackSigWE;
}
