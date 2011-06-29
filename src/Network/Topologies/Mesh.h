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

class Router;

class Mesh : public Network {

//==============================//
// Ports
//==============================//

// Inherited from Network:
//
//  sc_in<bool>   clock;
//
//  DataInput    *dataIn;
//  ReadyInput   *validDataIn;
//  ReadyOutput  *ackDataIn;
//
//  DataOutput   *dataOut;
//  ReadyOutput  *validDataOut;
//  ReadyInput   *ackDataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  Mesh(const sc_module_name& name,
       ComponentID ID,
       int rows,
       int columns,
       HierarchyLevel level,
       Dimension size);

  virtual ~Mesh();

//==============================//
// Methods
//==============================//

private:

  void makeRouters();
  void makeWires();
  void wireUp();

//==============================//
// Components
//==============================//

private:

  // 2D vector of routers. Indexed using routers[column][row]. (0,0) is in the
  // top left corner.
  std::vector<std::vector<Router*> > routers;

  // Lots of 2D arrays of signals. Each 2D array is indexed using
  // array[column][row]. Each array name is tagged with the direction it
  // carries data, e.g. NS = north to south.
  DataSignal  **dataSigNS,  **dataSigSN,  **dataSigEW,  **dataSigWE;
  ReadySignal **validSigNS, **validSigSN, **validSigEW, **validSigWE;
  ReadySignal **ackSigNS,   **ackSigSN,   **ackSigEW,   **ackSigWE;

//==============================//
// Local state
//==============================//

private:

  int numColumns, numRows;

};

#endif /* MESH_H_ */
