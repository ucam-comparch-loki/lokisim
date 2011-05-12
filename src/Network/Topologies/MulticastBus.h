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
//  DataInput     dataIn;
//  DataOutput   *dataOut;
//  ReadyOutput  *validOut;
//  ReadyInput   *ackIn;
//  ReadyOutput   canReceiveData;

  CreditInput  *creditsIn;
  ReadyOutput  *canReceiveCredits;

  CreditOutput  creditsOut;
  ReadyInput    canSendCredits;


//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(MulticastBus);

  // channelsPerOutput = number of channel IDs accessible through each output
  //                     of this bus. e.g. A memory may have 8 input channels,
  //                     which share only one input port.
  // startAddr         = the first channelID accessible through this network.
  MulticastBus(sc_module_name name, const ComponentID& ID, int numOutputs,
               int channelsPerOutput, const ChannelID& startAddr, Dimension size);
  virtual ~MulticastBus();

//==============================//
// Methods
//==============================//

protected:

  virtual void mainLoop();
  virtual void receivedData();

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
