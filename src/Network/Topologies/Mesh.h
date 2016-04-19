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

using std::vector;

class Mesh : public Network {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Network:
//
//  ClockInput   clock;
//
//  LokiVector<DataInput>  iData;
//  LokiVector<DataOutput> oData;

  // A signal from each router saying whether it is ready to receive data.
  LokiVector<ReadyOutput> oReady;

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

public:

  // Access methods to convert between the 2D arrangement of the mesh, and the
  // 1D arrangement of the standard network.
  DataInput& iData2D(uint x, uint y) const;
  DataOutput& oData2D(uint x, uint y) const;
  ReadyOutput& oReady2D(uint x, uint y) const;

  // Collections of signals which run off the edges of the Mesh.
  // Address using vector[Router::Direction][index].
  const vector<vector<DataSignal*> >  edgeDataInputs()   const;
  const vector<vector<DataSignal*> >  edgeDataOutputs()  const;
  const vector<vector<ReadySignal*> > edgeReadyOutputs() const;

private:

  void makeRouters();
  void makeWires();
  void wireUp();

  // Convert between 2D and 1D port addressing.
  uint flatten(uint x, uint y) const;

//============================================================================//
// Components
//============================================================================//

private:

  // 2D vector of routers. Indexed using routers[column][row]. (0,0) is in the
  // top left corner.
  vector<vector<Router*> > routers;

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

  // Collections of signals which run off the edges of the Mesh. These are
  // pointers to a subset of the signals above.
  // Address using vector[Router::Direction][index].
  vector<vector<DataSignal*> >  edgeDataInputs_;
  vector<vector<DataSignal*> >  edgeDataOutputs_;
  vector<vector<ReadySignal*> > edgeReadyOutputs_;

};

#endif /* MESH_H_ */
