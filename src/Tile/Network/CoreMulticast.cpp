/*
 * CoreMulticast.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "CoreMulticast.h"
#include "../../Utility/Assert.h"

using sc_core::sc_gen_unique_name;

CoreMulticast::CoreMulticast(const sc_module_name name,
                             const tile_parameters_t& params) :
    Network2<Word>(name, params.mcastNetInputs(), params.mcastNetOutputs()),
    outputsPerCore(params.core.numInputChannels),
    outputCores(params.numCores) {

}


PortIndex CoreMulticast::getDestination(const ChannelID address) const {
  // Single destination method should never be used.
  loki_assert(false);
  return -1;
}

set<PortIndex> CoreMulticast::getDestinations(const ChannelID address) const {
  loki_assert(address.multicast);

  set<PortIndex> destinations;

  for (uint core=0; core<outputCores; core++)
    if ((address.coremask >> core) & 1)
      destinations.insert(core*outputsPerCore + address.channel);

  return destinations;
}
