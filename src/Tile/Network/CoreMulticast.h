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

#include <vector>
#include "../../Network/Network.h"
#include "../../Network/Topologies/MulticastBus.h"
#include "../../Utility/LokiVector2D.h"

using std::vector;

class CoreMulticast: public Network {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Network:
//
//  ClockInput   clock;

  // Input data.
  LokiVector<DataInput>      iData;

  // Output data.
  // Addressed using oData[destination][source].
  LokiVector2D<DataOutput>   oData;

  // A signal from each buffer of each component, telling whether it is ready
  // to receive data. Addressed using iReady[component][buffer].
  LokiVector2D<ReadyInput>   iReady;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(CoreMulticast);
  CoreMulticast(const sc_module_name name, ComponentID tile,
                const tile_parameters_t& tileParams);

//============================================================================//
// Methods
//============================================================================//

private:

  // Have one copy of the main loop running for each input port.
  void mainLoop(PortIndex input);

//============================================================================//
// Local state
//============================================================================//

private:

  enum MulticastState {
    IDLE,
    FLOW_CONTROL,
    SEND,
    ACKNOWLEDGE,
  };

  vector<MulticastState> state;

  LokiVector<MulticastBus> buses;
  LokiVector<DataSignal> busInput;

};

#endif /* SRC_TILE_NETWORK_COREMULTICAST_H_ */
