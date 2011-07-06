/*
 * Router.h
 *
 * A simple 5-port router implementing XY-routing.
 *
 * There should be a single cycle between data being received, and it being
 * sent back onto the network.
 *
 *  Created on: 27 Jun 2011
 *      Author: db434
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include "../Component.h"
#include "NetworkTypedefs.h"
#include "../Memory/BufferArray.h"

class Router : public Component {

//==============================//
// Ports
//==============================//

public:

  enum Direction {NORTH, EAST, SOUTH, WEST, LOCAL};

  sc_in<bool>   clock;

  // Data inputs
  DataInput    *dataIn;
  ReadyInput   *validDataIn;
  ReadyOutput  *ackDataIn;

  // Data outputs
  DataOutput   *dataOut;
  ReadyOutput  *validDataOut;
  ReadyInput   *ackDataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Router);
  Router(const sc_module_name& name, const ComponentID& ID);
  virtual ~Router();

//==============================//
// Methods
//==============================//

private:

  // Receive input data, put it into buffers, and send acknowledgements.
  void receiveData(PortIndex input);

  // For the head of each input buffer, put the data into the appropriate
  // output buffer, if possible.
  void routeData();

  // Send data from output buffers onto the network and wait for
  // acknowledgements.
  void sendData();

  Direction routeTo(ChannelID destination) const;


//==============================//
// Components
//==============================//

private:

  BufferArray<DataType> inputBuffers, outputBuffers;

//==============================//
// Local state
//==============================//

private:

  // The position of this router in the grid of routers. Used to decide which
  // direction data should be sent next.
  const unsigned int xPos, yPos;

  // The router currently implements round-robin scheduling: store the inputs
  // which were most recently allowed to send data to each output.
  int lastAccepted[5];

};

#endif /* ROUTER_H_ */
