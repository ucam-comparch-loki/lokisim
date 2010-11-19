/*
 * Router.h
 *
 * Input/output 0 = to/from north
 * Input/output 1 = to/from east
 * Input/output 2 = to/from south
 * Input/output 3 = to/from west
 * Input/output 4 = to/from local network
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include "RoutingComponent.h"

class Router: public RoutingComponent {

//==============================//
// Ports
//==============================//

// Inherited from RoutingComponent:
//   clock
//   dataIn
//   dataOut
//   readyIn
//   readyOut

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Router);
  Router(sc_module_name name,
         ComponentID ID,
         int inputsPerTile,
         int networkType,
         Arbiter* arbiter);
  virtual ~Router();

//==============================//
// Methods
//==============================//

private:

  // Determine which output port should be used to reach the given destination.
  virtual ChannelIndex computeOutput(ChannelIndex source,
                                     ChannelID destination) const;

  virtual double distance(ChannelIndex inPort, ChannelIndex outPort) const;

//==============================//
// Local state
//==============================//

public:

  // The direction the signal will travel on each of the five output ports.
  enum Direction {NORTH, EAST, SOUTH, WEST, LOCAL};

private:

  // The position of this router in the global network.
  int column, row;

  // In order to determine which tile a destination channel is on, we need to
  // know how many destination channels there are on each tile. This can change
  // depending on whether this router deals with data or credits, and on which
  // level of the network hierarchy we are on.
  int inputsPerTile;

};

#endif /* ROUTER_H_ */
