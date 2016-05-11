/*
 * Mesh.cpp
 *
 *  Created on: 29 Jun 2011
 *      Author: db434
 */

#include "Mesh.h"
#include "../NetworkHierarchy.h"
#include "../Router.h"
#include "../../Utility/Assert.h"

DataInput&   Mesh::iData2D(uint x,  uint y) const {return iData[flatten(x,y)];}
DataOutput&  Mesh::oData2D(uint x,  uint y) const {return oData[flatten(x,y)];}
ReadyOutput& Mesh::oReady2D(uint x, uint y) const {return oReady[flatten(x,y)];}

const vector<vector<DataSignal*> > Mesh::edgeDataInputs() const {return edgeDataInputs_;}
const vector<vector<DataSignal*> > Mesh::edgeDataOutputs() const {return edgeDataOutputs_;}
const vector<vector<ReadySignal*> > Mesh::edgeReadyOutputs() const {return edgeReadyOutputs_;}

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

  // Take pointers to the signals around the edge of the chip and add them to
  // separate collections.
  for (uint row=0; row<numRows; row++) {
    edgeDataInputs_[Router::WEST].push_back(&dataSigWE[0][row]);
    edgeDataInputs_[Router::EAST].push_back(&dataSigEW[numColumns][row]);

    edgeDataOutputs_[Router::WEST].push_back(&dataSigEW[0][row]);
    edgeDataOutputs_[Router::EAST].push_back(&dataSigWE[numColumns][row]);

    edgeReadyOutputs_[Router::WEST].push_back(&readySigEW[0][row]);
    edgeReadyOutputs_[Router::EAST].push_back(&readySigWE[numColumns][row]);
  }

  for (uint col=0; col<numColumns; col++) {
    edgeDataInputs_[Router::NORTH].push_back(&dataSigNS[col][0]);
    edgeDataInputs_[Router::SOUTH].push_back(&dataSigSN[col][numRows]);

    edgeDataOutputs_[Router::NORTH].push_back(&dataSigSN[col][0]);
    edgeDataOutputs_[Router::SOUTH].push_back(&dataSigNS[col][numRows]);

    edgeReadyOutputs_[Router::NORTH].push_back(&readySigSN[col][0]);
    edgeReadyOutputs_[Router::SOUTH].push_back(&readySigNS[col][numRows]);
  }
}

void Mesh::wireUp() {
  for (unsigned int col=0; col<numColumns; col++) {
    for (unsigned int row=0; row<numRows; row++) {
      Router& router = *routers[col][row];
      router.clock(clock);

      // Data heading north-south
      router.iData[Router::NORTH](dataSigNS[col][row]);
      router.oData[Router::SOUTH](dataSigNS[col][row+1]);
      router.iReady[Router::NORTH](readySigNS[col][row]);
      router.oReady[Router::SOUTH](readySigNS[col][row+1]);

      // Data heading east-west
      router.iData[Router::EAST](dataSigEW[col+1][row]);
      router.oData[Router::WEST](dataSigEW[col][row]);
      router.iReady[Router::EAST](readySigEW[col+1][row]);
      router.oReady[Router::WEST](readySigEW[col][row]);

      // Data heading south-north
      router.iData[Router::SOUTH](dataSigSN[col][row+1]);
      router.oData[Router::NORTH](dataSigSN[col][row]);
      router.iReady[Router::SOUTH](readySigSN[col][row+1]);
      router.oReady[Router::NORTH](readySigSN[col][row]);

      // Data heading west-east
      router.iData[Router::WEST](dataSigWE[col][row]);
      router.oData[Router::EAST](dataSigWE[col+1][row]);
      router.iReady[Router::WEST](readySigWE[col][row]);
      router.oReady[Router::EAST](readySigWE[col+1][row]);

      // Data heading to/from local tile
      int tile = row*numColumns + col;
      router.iData[Router::LOCAL](iData[tile]);
      router.oData[Router::LOCAL](oData[tile]);
      router.oReady[Router::LOCAL](oReady[tile]);
    }
  }
}

uint Mesh::flatten(uint x, uint y) const {
  return (y*numColumns) + x;
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

  oReady.init(rows*columns);

  // Each set contains a vector for each of NORTH, EAST, SOUTH, WEST.
  edgeDataInputs_.assign(4, vector<DataSignal*>());
  edgeDataOutputs_.assign(4, vector<DataSignal*>());
  edgeReadyOutputs_.assign(4, vector<ReadySignal*>());

  makeRouters();
  makeWires();
  wireUp();

}

Mesh::~Mesh() {
  for (unsigned int i=0; i<routers.size(); i++)
    for (unsigned int j=0; j<routers[i].size(); j++)
      delete routers[i][j];
}
