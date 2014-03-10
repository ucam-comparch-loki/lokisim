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
#include "InstantCrossbar.h"
#include "MulticastNetwork.h"
#include "Crossbar.h"
#include "../../Communication/loki_bus.h"
#include "../../Utility/LokiVector2D.h"

class EndArbiter;

class LocalNetwork: public Network {

//==============================//
// Ports
//==============================//

public:

// Inherited from Network:
//
//  sc_in<bool>   clock;
//
//  DataInput    *dataIn;
//
//  DataOutput   *dataOut;
//  ReadyInput   *readyDataOut;

  // Additional clocks which are skewed, allowing multiple clocked events
  // to happen in series in one cycle.
  ClockInput   fastClock, slowClock;

  LokiVector<CreditInput>  creditsIn;
  LokiVector<CreditOutput> creditsOut;

  // An array of signals from each component, telling whether each of the
  // component's input buffers are ready to receive data.
  // Address using readyIn[component][buffer]
  LokiVector2D<ReadyInput>  readyDataIn;
  LokiVector2D<ReadyInput>  readyCreditsIn;

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

  // Inputs and outputs which connect to the global network.
  ReadyInput&   externalDataReady() const;
  ReadyInput&   externalCreditReady() const;

  CreditInput&  externalCreditIn() const;
  CreditOutput& externalCreditOut() const;

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
    GLOBAL_NETWORK
  };

  MulticastNetwork coreToCore;
  Crossbar coreToMemory, memoryToCore, coreToGlobal, globalToCore;

  // Don't want the new-style crossbar for credits - we don't know in advance
  // when we will need to send one (and the crossbar transition isn't forced to
  // fit in half a clock cycle).
  // Bring globalToCore down here too?
  InstantCrossbar c2gCredits, g2cCredits;

  // Data from each core to each of the subnetworks.
  // Addressed using dataSig[core][Destination]
  LokiVector2D<loki_bus> dataSig;

  // Signals allowing arbitration requests to be made for cores/memories/routers.
  // Currently the signals are written using a function call, but they can
  // be removed if we set up a proper SystemC channel connection.
  // Addressed using coreRequests[requester][destination]
  LokiVector2D<RequestSignal> coreRequests, memRequests, globalRequests;
  LokiVector2D<GrantSignal>   coreGrants,   memGrants,   globalGrants;

//==============================//
// Local state
//==============================//

private:

  // The numbers of ports required for credits. This is less than the number
  // required for data because credits are only used for some connections.
  static const unsigned int creditInputs;
  static const unsigned int creditOutputs;

};

#endif /* LOCALNETWORK_H_ */
