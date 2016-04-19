/*
 * MulticastBus.h
 *
 *  Created on: 8 Mar 2011
 *      Author: db434
 */

#ifndef MULTICASTBUS_H_
#define MULTICASTBUS_H_

#include "Bus.h"

class MulticastBus: public Bus {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Bus:
//
//  ClockInput   clock;
//
//  LokiVector<DataInput>  iData;
//  LokiVector<DataOutput> oData;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(MulticastBus);
  MulticastBus(const sc_module_name& name, const ComponentID& ID, int numOutputs,
               HierarchyLevel level, int firstOutput=0);
  virtual ~MulticastBus();

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual void busLoop();

private:

  void ackArrived(PortIndex port);

  // Compute which outputs of this bus will be used by the given address. This
  // allows an address to represent multiple destinations.
  // The PortIndex returned is a bitmask of the destinations to send to. The
  // least significant bit represents output 0.
  PortIndex getDestinations(const ChannelID& address) const;

//============================================================================//
// Local state
//============================================================================//

private:

  // The destination to send credits back to. We can't just forward all credits
  // received because there may be multiple credits for a single message.
//  ChannelID creditDestination;

  sc_core::sc_event receivedAllAcks;

};

#endif /* MULTICASTBUS_H_ */
