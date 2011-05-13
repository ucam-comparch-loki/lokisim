/*
 * LocalNetwork.h
 *
 * The data and credit networks within a single tile. Allows the cores to send
 * onto a multicast network whilst the memories send onto a normal crossbar.
 *
 * TODO: have multiple outputs from each core, one leading to other cores, one
 *       leading to memories, and one leading to the global network.
 * TODO: remove credits from the memory network once the new memory system is
 *       in place.
 *
 *  Created on: 9 May 2011
 *      Author: db434
 */

#ifndef LOCALNETWORK_H_
#define LOCALNETWORK_H_

#include "../Network.h"

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

  void makeWires();
  void makeArbiters();

  // firstPort is the position in this network's ports to which the arbiter
  // should be attached.
  void makeDataArbiter(ComponentID ID, unsigned int inputs, unsigned int outputs, unsigned int firstPort);
  void makeCreditArbiter(ComponentID ID, unsigned int inputs, unsigned int outputs, unsigned int firstPort);

  void makeBuses();

//==============================//
// Components
//==============================//

private:

  std::vector<ArbiterComponent*> arbiters;
  std::vector<Bus*> buses;

  // Lots of 2D arrays of wires to connect buses to arbiters.
  // Addressing is done as: dataSig[destination_component][port]
  DataSignal**   dataSig;
  ReadySignal**  validDataSig;
  ReadySignal**  ackDataSig;

  CreditSignal** creditSig;
  ReadySignal**  validCreditSig;
  ReadySignal**  ackCreditSig;

};

#endif /* LOCALNETWORK_H_ */
