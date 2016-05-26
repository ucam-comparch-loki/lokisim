/*
 * Network.cpp
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#include "Network.h"
#include "../Exceptions/InvalidOptionException.h"
#include "../Utility/Assert.h"

PortIndex Network::getDestination(ChannelID address, uint totalOutputs) const {
  PortIndex port;

  // Access a different part of the address depending on where in the network
  // we are.
  switch (level) {
    case TILE :
      port = address.component.tile.computeTileIndex() - firstOutput;
      break;
    case COMPONENT : {
      if (externalConnection && !(address.component.tile == id.tile))
        port = totalOutputs-1;
      else
        port = address.component.position - firstOutput;
      break;
    }
    case CHANNEL : {
      if (externalConnection && (address.component != id))
        port = totalOutputs-1;
      else
        port = address.channel - firstOutput;
      break;
    }
    case NONE :
      port = 0;
      break;
    default :
      port = 0;
      throw InvalidOptionException("network scope", level);
      break;
  }

  return port;
}

Network::Network(const sc_module_name& name,
    const ComponentID& ID,
    int numInputs,        // Number of inputs this network has
    int numOutputs,       // Number of outputs this network has
    HierarchyLevel level, // Position in the network hierarchy
    int firstOutput,      // The first accessible channel/component/tile
    bool externalConnection) : // Is there a port to send data on if it
                               // isn't for any local component?
    Component(name, ID),
    firstOutput(firstOutput),
    level((numOutputs > 1) ? level : NONE),
    externalConnection(externalConnection) {

  loki_assert(numInputs > 0);
  loki_assert(numOutputs > 0);

}
