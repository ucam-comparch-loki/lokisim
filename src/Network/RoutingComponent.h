/*
 * RoutingComponent.h
 *
 * The base class for all network components which dynamically choose where to
 * send data (e.g. Routers, Networks).
 *
 * This class specifies all of the required input and output ports, takes care
 * of link-level flow control (not end-to-end) and provides skeleton
 * implementations for collecting data and sending it out on the appropriate
 * port.
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#ifndef ROUTINGCOMPONENT_H_
#define ROUTINGCOMPONENT_H_

#include "../Component.h"
#include "../Memory/BufferArray.h"

class AddressedWord;
class Arbiter;

class RoutingComponent: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>            clock;

  sc_in<AddressedWord>  *dataIn;
  sc_out<AddressedWord> *dataOut;

  // Implement link-level flow control using ready signals. Note that this is
  // a completely separate mechanism to end-to-end flow control, which has its
  // own dedicated network.
  sc_in<bool>           *readyIn;
  sc_out<bool>          *readyOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(RoutingComponent);
  RoutingComponent(sc_module_name name, ComponentID ID, int numInputs,
                   int numOutputs, int bufferSize);
  virtual ~RoutingComponent();

//==============================//
// Methods
//==============================//

protected:

  // Determine which output port should be used to reach the given destination.
  virtual ChannelIndex computeOutput(ChannelIndex source,
                                     ChannelID destination) const = 0;

private:

  // Whenever data arrives, put it into the appropriate input buffer.
  void newData();

  // Try to send data to its destination whenever appropriate.
  void sendData();

  // Update one of our ready signals to say whether we have space to accept
  // more data at a particular input port.
  void updateReady(ChannelIndex input);

  void printDebugMessage(ChannelIndex from, ChannelIndex to, AddressedWord data) const;

//==============================//
// Methods
//==============================//

protected:

  Arbiter* arbiter_;

//==============================//
// Local state
//==============================//

protected:

  int numInputs, numOutputs;

private:

  // A flag to show if data has arrived in the last cycle. Avoids the need to
  // check every input for events.
  bool haveData;

  BufferArray<AddressedWord> inputBuffers;

};

#endif /* ROUTINGCOMPONENT_H_ */
