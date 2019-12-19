/*
 * CoreMulticast.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "CoreMulticast.h"
#include "../../Utility/Assert.h"

using sc_core::sc_gen_unique_name;
using std::set;

CoreMulticast::CoreMulticast(const sc_module_name name,
                             const tile_parameters_t& params) :
    Network<Word>(name, params.mcastNetInputs(), params.mcastNetOutputs()),
    outputsPerCore(params.core.numInputChannels),
    outputCores(params.numCores),
    outputsPerAccelerator(1),
    outputAccelerators(params.numAccelerators) {
	// Nothing
}


PortIndex CoreMulticast::getDestination(const ChannelID address) const {
  // Single destination method should never be used.
  loki_assert(false);
  return -1;
}

set<PortIndex> CoreMulticast::getDestinations(const ChannelID address) const {
  loki_assert(address.multicast);

  set<PortIndex> destinations;

  for (uint component=0; component<outputCores+outputAccelerators; component++) {
    if ((address.coremask >> component) & 1) {
      // Sending to a core.
      if (component < outputCores)
        destinations.insert(component*outputsPerCore + address.channel);
      // Sending to an accelerator.
      else
        destinations.insert(outputCores*outputsPerCore +
                            (component - outputCores) * outputsPerAccelerator +
                            address.channel);
    }
  }

  return destinations;
}
