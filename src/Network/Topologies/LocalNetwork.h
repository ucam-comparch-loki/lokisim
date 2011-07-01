/*
 * LocalNetwork.h
 *
 * The data and credit networks within a single tile. Allows the cores to send
 * onto a multicast network whilst the memories send onto a normal crossbar.
 *
 *  Created on: 9 May 2011
 *      Author: db434
 */

#ifndef LOCALNETWORK_H_
#define LOCALNETWORK_H_

#include "../Network.h"
#include <vector>

using std::vector;

class ArbiterComponent;
class Bus;

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

  LocalNetwork(sc_module_name name, ComponentID tile);
  virtual ~LocalNetwork();

//==============================//
// Methods
//==============================//

public:

  // The input port to this network which comes from the global network.
  CreditInput&  externalCreditIn() const;

  // The output port of this network which goes to the global network.
  CreditOutput& externalCreditOut() const;

  ReadyInput&   externalValidCreditIn() const;
  ReadyOutput&  externalValidCreditOut() const;

  ReadyInput&   externalAckCreditIn() const;
  ReadyOutput&  externalAckCreditOut() const;

private:

  // Create all of the arbiters required in this network.
  void makeArbiters();

  // Create all of the buses required in this network.
  void makeBuses();

  // Bind the relevant arbiter ports to this network's inputs/outputs.
  // port = the index in this network's port arrays to bind to.
  // data = flag telling whether the arbiter handles data or credits.
  void bindArbiter(ArbiterComponent* arbiter, PortIndex port, bool data);

  // Bind the relevant bus ports to this network's inputs/outputs.
  // port = the index in this network's port arrays to bind to.
  // data = flag telling whether the bus handles data or credits.
  void bindBus(Bus* bus, PortIndex port, bool data);

  // Connect the arbiters and buses together.
  void wireUp();

  // Connect a group of buses to a group of arbiters.
  // Typically, the ith output of bus j will connect to the jth input of
  // arbiter i. This can be adjusted using firstArbiterConnection if some
  // connections to the arbiter have already been made.
  // data is a flag telling whether the sub-network handles data or credits.
  void connect(vector<Bus*>& buses,
               vector<ArbiterComponent*>& arbiters,
               PortIndex firstArbiterConnection,
               bool data);

  // Create all signals necessary to connect one bus to one arbiter. They are
  // pushed onto the back of vectors, for later access.
  void makeDataSigs();

  // Create all signals necessary to connect one bus to one arbiter. They are
  // pushed onto the back of vectors, for later access.
  void makeCreditSigs();

//==============================//
// Components
//==============================//

private:

  unsigned int creditInputs, creditOutputs;

  // Buses and arbiters separated out according to the start/end components.
  // c = core, m = memory, g = global network
  // Note that wherever multicast-capable buses are used, credit arbiters are
  // not needed.
  vector<Bus*> c2cDataBuses, c2mDataBuses, c2gDataBuses, m2cDataBuses, g2cDataBuses;
  vector<Bus*> c2cCreditBuses, c2gCreditBuses, g2cCreditBuses;
  vector<ArbiterComponent*> cDataArbiters, mDataArbiters, gDataArbiters;
  vector<ArbiterComponent*> cCreditArbiters, gCreditArbiters;

  // There is a bus for each output port of each component. There need to be
  // wires between each bus and all arbiters which the bus may send to.
  // Pointers are stored here so they can all be deleted later.
  vector<DataSignal*>   dataSigs;
  vector<ReadySignal*>  validDataSigs;
  vector<ReadySignal*>  ackDataSigs;

  vector<CreditSignal*> creditSigs;
  vector<ReadySignal*>  validCreditSigs;
  vector<ReadySignal*>  ackCreditSigs;
};

#endif /* LOCALNETWORK_H_ */
