/*
 * Mesh.h
 *
 * A simple 2D mesh network.
 *
 * If any data is sent off the edges of the mesh, it is lost.
 *
 *  Created on: 29 Jun 2011
 *      Author: db434
 */

#ifndef MESH_H_
#define MESH_H_

#include "../Network.h"
#include "../Global/NetworkDeadEnd.h"

class Router;

using std::vector;

class Mesh : public Network {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Network:
//
//  ClockInput   clock;

  // Inputs to network (outputs from components).
  // Addressed using iData[column][row]
  LokiVector2D<DataInput>  iData;

  // Outputs from network (inputs to components).
  // Addressed using oData[column][row]
  LokiVector2D<DataOutput> oData;

  // A signal from each router saying whether it is ready to receive data.
  // Addressed using oReady[column][row]
  LokiVector2D<ReadyOutput> oReady;

  // A signal to each router saying whether it can send data to the local tile.
  // Addressed using iReady[column][row]
  LokiVector2D<ReadyOutput> iReady;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  Mesh(const sc_module_name& name,
       ComponentID ID,
       int rows,
       int columns,
       HierarchyLevel level);

  virtual ~Mesh();

//============================================================================//
// Methods
//============================================================================//

private:

  void makeRouters();
  void makeWires();
  void wireUp();

//============================================================================//
// Components
//============================================================================//

private:

  // 2D vector of routers. Indexed using routers[column][row]. (0,0) is in the
  // top left corner.
  vector<vector<Router*> > routers;

  // Debug components which warn us if data is sent off the edge of the network.
  vector<NetworkDeadEnd<NetworkData>*> edges;

  // Lots of 2D arrays of signals. Each 2D array is indexed using
  // array[column][row]. Each array name is tagged with the direction it
  // carries data, e.g. NS = north to south.
  LokiVector2D<DataSignal> dataSigNS, dataSigSN, dataSigEW, dataSigWE;
  LokiVector2D<ReadySignal> readySigNS, readySigSN, readySigEW, readySigWE;

//============================================================================//
// Local state
//============================================================================//

private:

  const unsigned int numColumns, numRows;

};

#endif /* MESH_H_ */
