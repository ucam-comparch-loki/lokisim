/*
 * CoreMulticast.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "CoreMulticast.h"
#include "../../Utility/Assert.h"
#include "../Accelerator/Accelerator.h"

using sc_core::sc_gen_unique_name;
using std::set;

CoreMulticast::CoreMulticast(const sc_module_name name,
                             const tile_parameters_t& params) :
    Network<Word>(name, params.mcastNetInputs(), params.mcastNetOutputs()),
    outputsPerCore(params.core.numInputChannels),
    outputsPerAccelerator(Accelerator::numMulticastInputs),
    outputCores(params.numCores),
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

  for (uint core=0; core<outputCores; core++)
    if ((address.coremask >> core) & 1)
      destinations.insert(core*outputsPerCore + address.channel);
  for (uint acc=0; acc<outputAccelerators; acc++)
    if ((address.coremask >> (acc + outputCores)) & 1)
      destinations.insert(outputCores*outputsPerCore +
                          acc*outputsPerAccelerator + address.channel);

  return destinations;
}
