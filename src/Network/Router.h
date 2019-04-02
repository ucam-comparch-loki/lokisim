/*
 * Router.h
 *
 * A simple 5-port router implementing XY-routing.
 *
 * There should be a single cycle between data being received, and it being
 * sent back onto the network.
 *
 * Data is always sent on the positive clock edge.
 *
 *  Created on: 27 Jun 2011
 *      Author: db434
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include "../LokiComponent.h"
#include "../Utility/LokiVector.h"
#include "FIFOs/FIFOArray.h"
#include "Network2.h"
#include "NetworkTypes.h"

template<typename T>
class RouterInternalNetwork;

template<typename T>
class Router : public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput clock;

  // The router is odd in that all of its ports are network sinks.
  // The inputs are attached to local buffers, so are real sinks.
  // The outputs are connected to other routers, so are remote sinks.
  typedef sc_port<network_sink_ifc<T>> InPort;
  typedef sc_port<network_sink_ifc<T>> OutPort;

  // Connections to other routers.
  LokiVector<InPort> inputs;
  LokiVector<OutPort> outputs;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  Router(const sc_module_name& name, const TileID& ID,
         const router_parameters_t& params);


//============================================================================//
// Components
//============================================================================//

private:

  FIFOArray<T> inputBuffers;
  RouterInternalNetwork<T> internal;

};

// An internal crossbar connecting all inputs to all outputs.
template<typename T>
class RouterInternalNetwork: public Network2<T> {
public:
  RouterInternalNetwork(const sc_module_name name, TileID tile);
  virtual ~RouterInternalNetwork();
  virtual PortIndex getDestination(const ChannelID address) const;
private:
  const TileID position;
};

#endif /* ROUTER_H_ */
