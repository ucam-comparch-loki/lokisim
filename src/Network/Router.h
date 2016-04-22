/*
 * Router.h
 *
 * A simple 5-port router implementing XY-routing.
 *
 * There should be a single cycle between data being received, and it being
 * sent back onto the network.
 *
 * Data is always sent on the positive clock edge.
 *
 *  Created on: 27 Jun 2011
 *      Author: db434
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include "../Component.h"
#include "../Utility/BlockingInterface.h"
#include "BufferArray.h"
#include "NetworkTypedefs.h"

class Router : public Component, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:

  enum Direction {NORTH, EAST, SOUTH, WEST, LOCAL};

  ClockInput   clock;

  // Data inputs
  LokiVector<DataInput>   iData;

  // A flow control signal to each neighbouring router and to the local network.
  LokiVector<ReadyOutput> oReady;

  // Data outputs
  LokiVector<DataOutput>  oData;

  // A flow control signal from each neighbouring router. Flow control from the
  // local network is handled separately.
  LokiVector<ReadyInput>  iReady;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(Router);
  Router(const sc_module_name& name, const ComponentID& ID);

//============================================================================//
// Methods
//============================================================================//

private:

  // Receive input data, put it into buffers, and send acknowledgements.
  void receiveData(PortIndex input);

  // Send data from output buffers onto the network and wait for
  // acknowledgements.
  void sendData(PortIndex output);

  // Update the flow control signal for the input port connected to the local
  // network.
  void updateFlowControl(PortIndex input);

  // Determine which output (if any) the head of a particular input buffer
  // needs to use.
  void updateDestination(PortIndex input);

  Direction routeTo(ChannelID destination) const;

  virtual void reportStalls(ostream& os);


//============================================================================//
// Components
//============================================================================//

private:

  BufferArray<NetworkData> inputBuffers;

//============================================================================//
// Local state
//============================================================================//

private:

  enum SendState {
    WAITING_FOR_DATA,
    ARBITRATING,
    WAITING_FOR_ACK
  };

  SendState state;

  // The position of this router in the grid of routers. Used to decide which
  // direction data should be sent next.
  const unsigned int xPos, yPos;

  // The router currently implements round-robin scheduling: store the inputs
  // which were most recently allowed to send data to each output.
  PortIndex lastAccepted[5];

  // Record the direction the leading flit in each buffer wants to travel.
  PortIndex destination[5];

  // Event to notify each output port when data is waiting to be sent.
  LokiVector<sc_event> outputAvailable;

};

#endif /* ROUTER_H_ */
