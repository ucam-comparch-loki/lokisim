/*
 * Router.cpp
 *
 *  Created on: 27 Jun 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Router.h"
#include "Topologies/LocalNetwork.h"

void Router::receiveData(PortIndex input) {
  if (DEBUG)
    cout << this->name() << ": input from " << (int)input << ": " << dataIn[input].read() << endl;

  assert(!inputBuffers[input].full());

//  if (inputBuffers[input].full()) {
//    // If the buffer is full, let the data sit on the wire until space becomes
//    // available.
//    // TODO: remove this, and rely on the flow control signals
//    next_trigger(inputBuffers[input].readEvent());
//  }
//  else {
    bool wasEmpty = inputBuffers[input].empty();

    // Put the new data into a buffer.
    inputBuffers[input].write(dataIn[input].read());
    dataIn[input].ack();

    if (wasEmpty)
      updateDestination(input);
//  }
}

void Router::localNetworkArbitration() {
  switch (state) {

    case WAITING_FOR_DATA:
      // Wait until there is data to be sent.
      if (!dataOut[LOCAL].valid())
        next_trigger(dataOut[LOCAL].default_event());
      else {
        // Issue a request for arbitration.
        localNetwork->makeRequest(id, dataOut[LOCAL].read().channelID(), true);
        state = ARBITRATING;
        next_trigger(clock.posedge_event());
      }
      break;

    case ARBITRATING:
      // Wait for the request to be granted.
      if (!localNetwork->requestGranted(id, dataOut[LOCAL].read().channelID())) {
        next_trigger(clock.posedge_event());
      }
      else {
        // Send the data and remove the arbitration request.
        const AddressedWord& dataToSend = dataOut[LOCAL].read();

        if (dataToSend.endOfPacket())
          localNetwork->makeRequest(id, dataToSend.channelID(), false);

        state = WAITING_FOR_ACK;
        next_trigger(dataOut[LOCAL].ack_event());
      }
      break;

    case WAITING_FOR_ACK:
      // Wait until the beginning of the next cycle before trying to send more data.
      state = WAITING_FOR_DATA;
      //next_trigger(sc_core::SC_ZERO_TIME);
      next_trigger(clock.posedge_event());
      break;

  }// end switch
}

void Router::sendData(PortIndex output) {
  // Wait for permission to send (connections to other routers only).
  if ((output != LOCAL) && !readyIn[output].read()) {
    next_trigger(readyIn[output].posedge_event());
  }
  // Data is always sent on the positive clock edge.
  else if (!clock.posedge()) {
    next_trigger(clock.posedge_event());
  }
  else {
    for (int i=0; i<5; i++) {
      PortIndex input = (i + lastAccepted[output] + 1) % 5;
      if (destination[input] == output) {
        if (DEBUG)
          cout << this->name() << " sending to " << (int)output << ": " << inputBuffers[input].peek() << endl;

        dataOut[output].write(inputBuffers[input].read());
        lastAccepted[output] = input;
        next_trigger(dataOut[output].ack_event());
        updateDestination(input);
        return;
      }
    }

    // If we couldn't find any data to send, wait until data arrives.
    next_trigger(outputAvailable[output]);
  }
}

void Router::updateFlowControl(PortIndex input) {
  bool canReceive = !inputBuffers[input].full();
  if (readyOut[input].read() != canReceive) {
    readyOut[input].write(canReceive);
  }
}

void Router::updateDestination(PortIndex input) {
  PortIndex output = -1;

  if (!inputBuffers[input].empty()) {
    output = routeTo(inputBuffers[input].peek().channelID());
    outputAvailable[output].notify();
  }

  destination[input] = output;
}

Router::Direction Router::routeTo(ChannelID destination) const {
  // Figure out where in the grid of tiles the destination channel end is.
  unsigned int xDest = destination.getTile() % NUM_TILE_COLUMNS;
  unsigned int yDest = destination.getTile() / NUM_TILE_COLUMNS;

  // XY routing: change x until we are in the right column, then change y.
  if (xDest > xPos)      return EAST;
  else if (xDest < xPos) return WEST;
  else if (yDest > yPos) return SOUTH;
  else if (yDest < yPos) return NORTH;
  else                   return LOCAL;
}

void Router::reportStalls(ostream& os) {
  for (uint i=0; i<dataOut.length(); i++) {
    if (dataOut[i].valid()) {
      os << this->name() << ".output_" << i << " is blocked." << endl;
      os << "  Target destination is " << dataOut[i].read().channelID() << endl;
    }
  }

  for (uint i=0; i<dataIn.length(); i++) {
    if (inputBuffers[i].full()) {
      os << inputBuffers[i].name() << " is full." << endl;
      os << "  Head is trying to get to " << inputBuffers[i].peek().channelID() << endl;
    }
  }
}

Router::Router(const sc_module_name& name, const ComponentID& ID, const bool carriesCredits, local_net_t* network) :
    Component(name, ID),
    Blocking(),
    inputBuffers(5, ROUTER_BUFFER_SIZE, string(this->name()) + ".input_data"),
    xPos(ID.getTile() % NUM_TILE_COLUMNS),
    yPos(ID.getTile() / NUM_TILE_COLUMNS),
    carriesCredits(carriesCredits),
    localNetwork(network) {

  state = WAITING_FOR_DATA;

  for (int i=0; i<5; i++) {
    lastAccepted[i] = -1;
    destination[i] = -1;
  }

  dataIn.init(5);    dataOut.init(5);
  readyIn.init(4);   readyOut.init(5);
  outputAvailable.init(5);

  // Generate a method to watch each input port, putting the data into the
  // appropriate buffer when it arrives.
  // Generate a method for each output port, sending data when there is data
  // to send.
  for (size_t i=0; i<dataIn.length(); i++) {
    SPAWN_METHOD(dataIn[i], Router::receiveData, i, false);
    SPAWN_METHOD(outputAvailable[i], Router::sendData, i, false);

    // Need to do this the long way because it's sensitive to multiple events.
    sc_core::sc_spawn_options options;
    options.spawn_method();     /* Want an efficient method, not a thread */
    options.set_sensitivity(&(inputBuffers[i].readEvent()));
    options.set_sensitivity(&(inputBuffers[i].writeEvent()));
    sc_spawn(sc_bind(&Router::updateFlowControl, this, i), 0, &options);
  }

  if (!carriesCredits) {
    SC_METHOD(localNetworkArbitration);
    // do initialise
  }

//  SC_METHOD(updateFlowControl);
//  sensitive << inputBuffers[LOCAL].readEvent() << inputBuffers[LOCAL].writeEvent();
//  // do initialise

}
