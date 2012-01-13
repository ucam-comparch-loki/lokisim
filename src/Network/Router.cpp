/*
 * Router.cpp
 *
 *  Created on: 27 Jun 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Router.h"

void Router::receiveData(PortIndex input) {
//  if(clock.posedge()) {
//    // Wait a delta cycle in case the valid signal is deasserted.
//    next_trigger(sc_core::SC_ZERO_TIME);
//  }
//  else {
    if(!dataIn[input].valid()) {
      // The valid signal was deasserted - wait until it goes high again.
      next_trigger(dataIn[input].default_event());
    }
    else if(inputBuffers[input].full()) {
      // There is no space to read the value from the network - wait a clock
      // cycle and check again.
      next_trigger(clock.posedge_event());
    }
    else {
      // Put the new data into a buffer.
      inputBuffers[input].write(dataIn[input].read());
      dataIn[input].ack();

      // Wait until the clock edge to deassert the acknowledgement.
      next_trigger(clock.posedge_event());
    }
//  }
}

void Router::routeData() {
  if(clock.posedge()) {
    // Wait a delta cycle to allow data to enter buffers.
    next_trigger(sc_core::SC_ZERO_TIME);
  }
  else {
    // TODO: only do this if we know there is at least some data in the input
    // buffers.

    // Loop through all output buffers, seeing which inputs want to use each
    // one. Select at most one input.
    for(int output=0; output<5; output++) {
      if(!outputBuffers[output].full()) {
        for(int i=0; i<5; i++) {
          // Implement a wrap-around counter, starting at the last accepted input.
          int input = (lastAccepted[output] + i + 1) % 5;

          // If there is data in this input buffer, and it wants to go to this
          // output buffer, send it.
          if(!inputBuffers[input].empty() &&
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

void Router::sendData() {
  for(int i=0; i<5; i++) {
    // We can send data if there is data to send, and there is no unacknowledged
    // data already on the output port.
    if(!outputBuffers[i].empty() && !dataOut[i].valid()) {
      dataOut[i].write(outputBuffers[i].read());
    }
  }
}

Router::Direction Router::routeTo(ChannelID destination) const {
  // Figure out where in the grid of tiles the destination channel end is.
  unsigned int xDest = destination.getTile() % NUM_TILE_COLUMNS;
  unsigned int yDest = destination.getTile() / NUM_TILE_COLUMNS;

  // XY routing: change x until we are in the right column, then change y.
  if(xDest > xPos)      return EAST;
  else if(xDest < xPos) return WEST;
  else if(yDest > yPos) return SOUTH;
  else if(yDest < yPos) return NORTH;
  else                  return LOCAL;
}

Router::Router(const sc_module_name& name, const ComponentID& ID) :
    Component(name, ID),
    inputBuffers(5, ROUTER_BUFFER_SIZE, "input_data"),
    outputBuffers(5, ROUTER_BUFFER_SIZE, "output_data"),
    xPos(ID.getTile() % NUM_TILE_COLUMNS),
    yPos(ID.getTile() / NUM_TILE_COLUMNS) {

  for(int i=0; i<5; i++) lastAccepted[i] = -1;

  dataIn = new DataInput[5];    dataOut = new DataOutput[5];

  // Generate a method to watch each input port, putting the data into the
  // appropriate buffer when it arrives.
  for(int i=0; i<5; i++)
    SPAWN_METHOD(dataIn[i], Router::receiveData, i, false);

  SC_METHOD(routeData);
  sensitive << clock.pos(); // Plus a delta cycle or two
  dont_initialize();

  // TODO: use a dynamic process for each output port, like the ones for input
  // ports above.
  SC_METHOD(sendData);
  sensitive << clock.pos();
  dont_initialize();

}

Router::~Router() {
  delete[] dataIn;
  delete[] dataOut;
}
