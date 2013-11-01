/*
 * Router.cpp
 *
 * TODO: remove output buffers
 * TODO: more dynamic processes
 *
 *  Created on: 27 Jun 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Router.h"
#include "Topologies/LocalNetwork.h"

void Router::receiveData(PortIndex input) {
  if (DEBUG)
    cout << this->name() << " received data on input " << (int)input << endl;

  assert(!inputBuffers[input].full());

  // Put the new data into a buffer.
  inputBuffers[input].write(dataIn[input].read());
  dataIn[input].ack();
}

void Router::routeData() {
  if (clock.posedge()) {
    // Wait a delta cycle to allow data to enter buffers.
    next_trigger(sc_core::SC_ZERO_TIME);
  }
  else {
    // TODO: only do this if we know there is at least some data in the input
    // buffers.

    // Loop through all output buffers, seeing which inputs want to use each
    // one. Select at most one input.
    for (int output=0; output<5; output++) {
      if (!outputBuffers[output].full()) {
        for (int i=0; i<5; i++) {
          // Implement a wrap-around counter, starting at the last accepted input.
          int input = (lastAccepted[output] + i + 1) % 5;

          // If there is data in this input buffer, and it wants to go to this
          // output buffer, send it.
          if (!inputBuffers[input].empty() &&
              routeTo(inputBuffers[input].peek().channelID()) == output) {
             outputBuffers[output].write(inputBuffers[input].read());
            break;
          }
        }
      }
    }

    next_trigger(clock.posedge_event());
  }
}

void Router::sendToLocalNetwork() {
  switch (state) {

    case WAITING_FOR_DATA:
      // Wait until there is data in the buffer.
      if (outputBuffers[LOCAL].empty())
        next_trigger(outputBuffers[LOCAL].writeEvent());
      else if (carriesCredits) {
        // Credits don't require arbitration, so skip to WAITING_FOR_ACK.
        dataOut[LOCAL].write(outputBuffers[LOCAL].read());

        state = WAITING_FOR_ACK;
        next_trigger(dataOut[LOCAL].ack_event());
      }
      else {
        // Issue a request for arbitration.
        localNetwork->makeRequest(id, outputBuffers[LOCAL].peek().channelID(), true);
        state = ARBITRATING;
        next_trigger(clock.posedge_event());
      }
      break;

    case ARBITRATING:
      // Wait for the request to be granted.
      if (!localNetwork->requestGranted(id, outputBuffers[LOCAL].peek().channelID())) {
        next_trigger(clock.posedge_event());
      }
      else {
        // Send the data and remove the arbitration request.
        const AddressedWord& dataToSend = outputBuffers[LOCAL].read();

        if (dataToSend.endOfPacket())
          localNetwork->makeRequest(id, dataToSend.channelID(), false);

        dataOut[LOCAL].write(dataToSend);

        state = WAITING_FOR_ACK;
        next_trigger(dataOut[LOCAL].ack_event());
      }
      break;

    case WAITING_FOR_ACK:
      // Wait until the beginning of the next cycle before trying to send more data.
      state = WAITING_FOR_DATA;
      next_trigger(sc_core::SC_ZERO_TIME);
      break;

  }// end switch
}

void Router::sendData() {
  for (int i=0; i<4; i++) {
    // We can send data if there is data to send, and there is no unacknowledged
    // data already on the output port.
    if (!outputBuffers[i].empty() && !dataOut[i].valid()) {
      dataOut[i].write(outputBuffers[i].read());
    }
  }
}

void Router::updateFlowControl() {
  bool canReceive = !inputBuffers[LOCAL].full();
  if (readyOut.read() != canReceive)
    readyOut.write(canReceive);
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

Router::Router(const sc_module_name& name, const ComponentID& ID, const bool carriesCredits, local_net_t* network) :
    Component(name, ID),
    inputBuffers(5, ROUTER_BUFFER_SIZE, "input_data"),
    outputBuffers(5, ROUTER_BUFFER_SIZE, "output_data"),
    xPos(ID.getTile() % NUM_TILE_COLUMNS),
    yPos(ID.getTile() / NUM_TILE_COLUMNS),
    carriesCredits(carriesCredits),
    localNetwork(network) {

  state = WAITING_FOR_DATA;

  for (int i=0; i<5; i++) lastAccepted[i] = -1;

  dataIn.init(5);    dataOut.init(5);

  // Generate a method to watch each input port, putting the data into the
  // appropriate buffer when it arrives.
  for (size_t i=0; i<dataIn.length(); i++)
    SPAWN_METHOD(dataIn[i], Router::receiveData, i, false);

  SC_METHOD(routeData);
  sensitive << clock.pos(); // Plus a delta cycle or two
  dont_initialize();

  // TODO: use a dynamic process for each output port, like the ones for input
  // ports above.
  SC_METHOD(sendData);
  sensitive << clock.pos();
  dont_initialize();

  SC_METHOD(sendToLocalNetwork);
  // do initialise

  SC_METHOD(updateFlowControl);
  sensitive << inputBuffers[LOCAL].readEvent() << inputBuffers[LOCAL].writeEvent();
  // do initialise

}
