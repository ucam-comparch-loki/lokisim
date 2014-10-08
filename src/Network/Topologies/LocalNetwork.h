/*
 * LocalNetwork.h
 *
 * The network within a single tile. Contains various subnetworks for
 * communicating between different classes of components, and communicating
 * different information.
 *
 * Also contains a link to be connected to the global network.
 *
 *  Created on: 9 Sep 2011
 *      Author: db434
 */

#ifndef LOCALNETWORK_H_
#define LOCALNETWORK_H_

#include "../Network.h"
#include "MulticastNetwork.h"
#include "Crossbar.h"
#include "../../Communication/loki_bus.h"
#include "../../Utility/LokiVector2D.h"

class LocalNetwork: public Network {

//==============================//
// Ports
//==============================//

public:

// Inherited from Network:
//
//  ClockInput   clock;
//
//  LokiVector<DataInput>  iData;
//  LokiVector<DataOutput> oData;

  // Additional clocks which are skewed, allowing multiple clocked events
  // to happen in series in one cycle.
  ClockInput   fastClock, slowClock;

  // An array of signals from each component, telling whether each of the
  // component's input buffers are ready to receive data.
  // Address using readyIn[component][buffer]
  LokiVector2D<ReadyInput>  iReady;

//==============================//
// Constructors and destructors
//==============================//

public:

  LocalNetwork(const sc_module_name& name, ComponentID tile);

//==============================//
// Methods
//==============================//

public:

  // Issue a request for arbitration. This should only be called for the first
  // and last flits of each packet.
  void makeRequest(ComponentID source, ChannelID destination, bool request);

  // See if the request from source to destination has been granted.
  bool requestGranted(ComponentID source, ChannelID destination) const;

private:

  // Helper methods for makeRequest.
  void multicastRequest(ComponentID source, ChannelID destination, bool request);
  void pointToPointRequest(ComponentID source, ChannelID destination, bool request);

  void createSignals();
  void wireUpSubnetworks();

  // Method triggered whenever a core sends new data onto the network.
  void newCoreData(int core);
  void coreDataAck(int core);

//==============================//
// Components
//==============================//

private:

  // The possible component types to which data can be sent.
  enum Destination {
    CORE,
    MEMORY,
  };

  MulticastNetwork coreToCore;
  Crossbar coreToMemory, memoryToCore;

  // Data from each core to each of the subnetworks.
  // Addressed using dataSig[core][Destination]
  LokiVector2D<loki_bus> dataSig;

  // Signals allowing arbitration requests to be made for cores/memories/routers.
  // Currently the signals are written using a function call, but they can
  // be removed if we set up a proper SystemC channel connection.
  // Addressed using coreRequests[requester][destination]
  LokiVector2D<ArbiterRequestSignal> coreRequests, memRequests;
  LokiVector2D<ArbiterGrantSignal>   coreGrants,   memGrants;

};

#endif /* LOCALNETWORK_H_ */
