/*
 * CoreMulticast.h
 *
 * Network allowing multicast between cores on a single tile.
 *
 * A single-writer policy is assumed, so the result of two sources
 * simultaneously sending to the same destination is undefined. No arbitration
 * is provided.
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_COREMULTICAST_H_
#define SRC_TILE_NETWORK_COREMULTICAST_H_

#include "../../Network/Network.h"

class CoreMulticast: public Network<Word> {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Network:
//
//  ClockInput clock;
//
//  LokiVector<InPort> inputs;    // One per core
//  LokiVector<OutPort> outputs;  // One per core input buffer
//                                // Numbered core*(buffers per core) + buffer

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  CoreMulticast(const sc_module_name name,
                const tile_parameters_t& tileParams);

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual PortIndex getDestination(const ChannelID address) const;
  virtual set<PortIndex> getDestinations(const ChannelID address) const;

//============================================================================//
// Local state
//============================================================================//

private:

  // The number of output ports which lead to the same core.
  const uint outputsPerCore;

  // Number of cores reachable through outputs.
  const uint outputCores;

};

#endif /* SRC_TILE_NETWORK_COREMULTICAST_H_ */
