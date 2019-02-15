/*
 * Network.cpp
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#include "Network.h"
#include "../Utility/Assert.h"

PortIndex Network::getDestination(ChannelID address) const {
  loki_assert_with_message(false, "No implementation for Network::getDestination", 0);
  return -1;
}

Network::Network(const sc_module_name& name) :
    LokiComponent(name) {

  // Nothing.

}
