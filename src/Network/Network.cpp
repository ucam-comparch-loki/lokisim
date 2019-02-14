/*
 * Network.cpp
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#include "Network.h"

#include "../Chip.h"
#include "../Exceptions/InvalidOptionException.h"
#include "../Utility/Assert.h"

PortIndex Network::getDestination(ChannelID address) const {
  loki_assert_with_message(false, "No implementation for Network::getDestination", 0);
  return -1;
}

Network::Network(const sc_module_name& name,
    int numInputs,        // Number of inputs this network has
    int numOutputs,       // Number of outputs this network has
    int firstOutput,      // The first accessible channel/component/tile
    bool externalConnection) : // Is there a port to send data on if it
                               // isn't for any local component?
    LokiComponent(name),
    clock("clock"),
    firstOutput(firstOutput),
    externalConnection(externalConnection) {

  loki_assert(numInputs > 0);
  loki_assert(numOutputs > 0);

}
