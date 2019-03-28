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

#include "../../Utility/LokiVector2D.h"
#include "../Network.h"
#include "../Global/NetworkDeadEnd2.h"
#include "../Router.h"

class Mesh : public Network {

//============================================================================//
// Ports
//============================================================================//

public:

  // Routers consume their inputs on the positive clock edge.
  ClockInput   clock;

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
       size2d_t size,
       const router_parameters_t& routerParams);

//============================================================================//
// Methods
//============================================================================//

private:

  void makeRouters(size2d_t tiles, const router_parameters_t& params);
  void wireUp(size2d_t tiles);

//============================================================================//
// Components
//============================================================================//

private:

  // 2D vector of routers. Indexed using routers[column][row]. (0,0) is in the
  // top left corner.
  LokiVector2D<Router<Word>> routers;

  // Debug components which warn us if data is sent off the edge of the network.
  LokiVector<NetworkDeadEnd2<Word>> edges;

};

#endif /* MESH_H_ */
