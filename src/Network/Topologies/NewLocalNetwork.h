/*
 * NewLocalNetwork.h
 *
 *  Created on: 9 Sep 2011
 *      Author: db434
 */

#ifndef NEWLOCALNETWORK_H_
#define NEWLOCALNETWORK_H_

#include "../Network.h"
#include "Crossbar.h"
#include "NewCrossbar.h"

class NewLocalNetwork: public Network {

//==============================//
// Ports
//==============================//

public:

// Inherited from Network:
//
//  sc_in<bool>   clock;
//
//  DataInput    *dataIn;
//  ReadyInput   *validDataIn;
//  ReadyOutput  *ackDataIn;
//
//  DataOutput   *dataOut;
//  ReadyOutput  *validDataOut;
//  ReadyInput   *ackDataOut;

  CreditInput  *creditsIn;
  ReadyInput   *validCreditIn;
  ReadyOutput  *ackCreditIn;

  CreditOutput *creditsOut;
  ReadyOutput  *validCreditOut;
  ReadyInput   *ackCreditOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  NewLocalNetwork(const sc_module_name& name, ComponentID tile);
  virtual ~NewLocalNetwork();

//==============================//
// Methods
//==============================//

public:

  // Issue a request for arbitration. This should only be called for the first
  // and last flits of each packet.
  // Returns an event which will be triggered when the request is granted.
  const sc_event& makeRequest(ComponentID source, ChannelID destination, bool request);

  // Inputs and outputs which connect to the global network.
  CreditInput&  externalCreditIn() const;
  CreditOutput& externalCreditOut() const;
  ReadyInput&   externalValidCreditIn() const;
  ReadyOutput&  externalValidCreditOut() const;
  ReadyInput&   externalAckCreditIn() const;
  ReadyOutput&  externalAckCreditOut() const;

private:

  void createArbiters();
  void createMuxes();
  void createSignals();
  void wireUpSubnetworks();

//==============================//
// Components
//==============================//

private:

  NewCrossbar coreToCore, coreToMemory, memoryToCore, coreToGlobal, globalToCore;

  // Don't want the new-style crossbar for credits - we don't know in advance
  // when we will need to send one (and the crossbar transition isn't forced to
  // fit in half a clock cycle).
  // Bring globalToCore down here too?
  Crossbar c2gCredits, g2cCredits;

  std::vector<BasicArbiter*> arbiters;
  std::vector<Multiplexer*>  muxes;

  // Signals connecting to muxes.
  // Address using dataSig[destination component][position]
  //   where coreToCore connects to positions 0 and 1,
  //         memoryToCore connects to positions 2 and 3,
  //         globalToCore connects to position 4
  DataSignal               **dataSig;
  ReadySignal              **validSig;
  ReadySignal              **ackSig;

  // Signals connecting arbiters to arbiters.
  // Address using selectSig[arbiter][port]
  sc_signal<int>           **selectSig;
  sc_signal<bool>          **requestSig;
  sc_signal<bool>          **grantSig;

  // Signals allowing arbitration requests to be made for cores/memories/routers.
  // Currently the signals are written using a function call, but they can
  // be removed if we set up a proper SystemC channel connection.
  // Addressed using coreRequests[requester][destination]
  sc_signal<bool>          **coreRequests, **memRequests, **globalRequests;
  sc_signal<bool>          **coreGrants,   **memGrants,   **globalGrants;

//==============================//
// Local state
//==============================//

private:

  // The numbers of ports required for credits. This is less than the number
  // required for data because credits are only used for some connections.
  static const unsigned int creditInputs;
  static const unsigned int creditOutputs;

  // The number of inputs to each arbiter/mux in this network.
  static const unsigned int muxInputs;

};

#endif /* NEWLOCALNETWORK_H_ */
