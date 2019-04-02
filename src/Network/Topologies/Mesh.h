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
#include "../Global/NetworkDeadEnd.h"
#include "../Router.h"

class Mesh : public Network {

//============================================================================//
// Ports
//============================================================================//

public:

  // Routers consume their inputs on the positive clock edge.
  ClockInput   clock;

  // Both inputs and outputs count as network sinks. The inputs are sinks of the
  // local tile networks, and the outputs are the sinks of this mesh network.
  typedef sc_port<network_sink_ifc<Word>> InPort;
  typedef sc_port<network_sink_ifc<Word>> OutPort;

  // Inputs from tiles.
  // Addressed using inputs[column][row].
  LokiVector2D<InPort> inputs;

  // Outputs to tiles.
  // Addressed using outputs[column][row].
  LokiVector2D<OutPort> outputs;

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

  void makeComponents(size2d_t tiles, const router_parameters_t& params);
  void wireUp(size2d_t tiles);

//============================================================================//
// Components
//============================================================================//

private:

  // 2D vector of routers. Indexed using routers[column][row]. (0,0) is in the
  // top left corner.
  LokiVector2D<Router<Word>> routers;

  // Debug components which warn us if data is sent off the edge of the network.
  LokiVector<NetworkDeadEnd<Word>> edges;

};

#endif /* MESH_H_ */
