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

#include "../LokiComponent.h"
#include "../Utility/BlockingInterface.h"
#include "../Utility/LokiVector.h"
#include "FIFOs/FIFOArray.h"
#include "NetworkTypes.h"

class Router : public LokiComponent, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:

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
  Router(const sc_module_name& name, const TileID& ID,
         const router_parameters_t& params);

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

  FIFOArray<NetworkData> inputBuffers;

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
  const TileID position;

  // Whether this router is using wormhole routing.
  bool wormhole;

  // The router currently implements round-robin scheduling: store the inputs
  // which were most recently allowed to send data to each output.
  PortIndex lastAccepted[5];

  // If using wormhole routing, allow the same input to be used until the
  // packet ends.
  bool holdInput[5];

  // Record the direction the leading flit in each buffer wants to travel.
  PortIndex destination[5];

  // Event to notify each output port when data is waiting to be sent.
  LokiVector<sc_event> outputAvailable;

};

#endif /* ROUTER_H_ */
