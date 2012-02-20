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

#ifndef NEWLOCALNETWORK_H_
#define NEWLOCALNETWORK_H_

#include "../NewNetwork.h"
#include "Crossbar.h"
#include "NewCrossbar.h"

class EndArbiter;

class LocalNetwork: public NewNetwork {

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
  sc_in<bool>   fastClock, slowClock;

  CreditInput  *creditsIn;
  CreditOutput *creditsOut;

  // An array of signals from each component, telling whether each of the
  // component's input buffers are ready to receive data.
  ReadyInput  **readyIn;

//==============================//
// Constructors and destructors
//==============================//

public:

  LocalNetwork(const sc_module_name& name, ComponentID tile);
  virtual ~LocalNetwork();

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
  ReadyInput&   externalReadyInput() const;

  CreditInput&  externalCreditIn() const;
  CreditOutput& externalCreditOut() const;

private:

  void createSignals();
  void wireUpSubnetworks();

//==============================//
// Components
//==============================//

private:
  // FIXME: this signal is unused - delete it.
  // Seems to cause segfaults if removed (or moved any lower than this).
  sc_signal<bool> deleteme;

private:

  NewCrossbar coreToCore, coreToMemory, memoryToCore, coreToGlobal, globalToCore;

  // Don't want the new-style crossbar for credits - we don't know in advance
  // when we will need to send one (and the crossbar transition isn't forced to
  // fit in half a clock cycle).
  // Bring globalToCore down here too?
  Crossbar c2gCredits, g2cCredits;

  // Signals allowing arbitration requests to be made for cores/memories/routers.
  // Currently the signals are written using a function call, but they can
  // be removed if we set up a proper SystemC channel connection.
  // Addressed using coreRequests[requester][destination]
  RequestSignal            **coreRequests, **memRequests, **globalRequests;
  GrantSignal              **coreGrants,   **memGrants,   **globalGrants;

//==============================//
// Local state
//==============================//

private:

  // The numbers of ports required for credits. This is less than the number
  // required for data because credits are only used for some connections.
  static const unsigned int creditInputs;
  static const unsigned int creditOutputs;

  // If a connection is already set up between two components, when a request
  // is made, it will not be granted again.
  // Use this event to trigger any methods which depend on the request being
  // granted.
  // To be removed when the request/grant interface is integrated into
  // components properly.
  sc_event grantEvent;

};

#endif /* NEWLOCALNETWORK_H_ */
