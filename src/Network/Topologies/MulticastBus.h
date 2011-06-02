/*
 * MulticastBus.h
 *
 *  Created on: 8 Mar 2011
 *      Author: db434
 */

#ifndef MULTICASTBUS_H_
#define MULTICASTBUS_H_

#include "Bus.h"
#include <list>

class MulticastBus: public Bus {

//==============================//
// Ports
//==============================//

public:

// Inherited from Bus:
//
//   sc_in<bool>   clock;
//
//   DataInput    *dataIn;
//   ReadyInput   *validDataIn;
//   ReadyOutput  *ackDataIn;
//
//   DataOutput   *dataOut;
//   ReadyOutput  *validDataOut;
//   ReadyInput   *ackDataOut;

  CreditInput  *creditsIn;
  ReadyInput   *validCreditIn;
  ReadyOutput  *ackCreditIn;

  // Make these into arrays of length 1, for consistency?
  CreditOutput  creditsOut;
  ReadyOutput   validCreditOut;
  ReadyInput    ackCreditOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(MulticastBus);
  MulticastBus(sc_module_name name, const ComponentID& ID, int numOutputs,
               HierarchyLevel level, Dimension size);
  virtual ~MulticastBus();

//==============================//
// Methods
//==============================//

protected:

  virtual void mainLoop();
  virtual void receivedData();
  virtual void receivedCredit(PortIndex output);

private:

  void checkCredits();
  void creditArrived();
  // Compute which outputs of this bus will be used by the given address. This
  // allows an address to represent multiple destinations.
  void getDestinations(const ChannelID& address, std::vector<PortIndex>& outputs) const;

//==============================//
// Local state
//==============================//

private:

  // Multicast is complicated unless we keep track of which outputs owe credits.
  std::list<PortIndex> outstandingCredits;

  // The destination to send credits back to. We can't just forward all credits
  // received because there may be multiple credits for a single message.
  ChannelID creditDestination;

  sc_core::sc_event credit;

};

#endif /* MULTICASTBUS_H_ */
