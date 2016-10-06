/*
 * Router.cpp
 *
 *  Created on: 27 Jun 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Router.h"
#include "Topologies/LocalNetwork.h"

const string DirectionNames[] = {"north", "east", "south", "west", "local"};

void Router::receiveData(PortIndex input) {
  if (inputBuffers[input].full()) {
    // If the buffer is full, let the data sit on the wire until space becomes
    // available.
    // TODO: remove this, and rely on the flow control signals
    next_trigger(inputBuffers[input].readEvent());
  }
  else {
    LOKI_LOG << this->name() << ": input from " << DirectionNames[input] << ": " << iData[input].read() << endl;
    bool wasEmpty = inputBuffers[input].empty();

    // Put the new data into a buffer.
    inputBuffers[input].write(iData[input].read());
    iData[input].ack();

    if (wasEmpty)
      updateDestination(input);
  }
}

void Router::sendData(PortIndex output) {
  // Wait for permission to send.
  if (!iReady[output].read()) {
    next_trigger(iReady[output].posedge_event());
  }
  // Data is always sent on the positive clock edge.
  else if (!clock.posedge()) {
    next_trigger(clock.posedge_event());
  }
  else {
    for (int i=0; i<5; i++) {
      PortIndex input = (i + lastAccepted[output] + 1) % 5;
      if (destination[input] == output) {
        LOKI_LOG << this->name() << " sending to " << DirectionNames[output] << ": " << inputBuffers[input].peek() << endl;

        oData[output].write(inputBuffers[input].read());
        lastAccepted[output] = input;
        next_trigger(oData[output].ack_event());
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
  if (oReady[input].read() != canReceive) {
    oReady[input].write(canReceive);
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

Direction Router::routeTo(ChannelID destination) const {
  // Figure out where in the grid of tiles the destination channel end is.
  unsigned int xDest = destination.component.tile.x;
  unsigned int yDest = destination.component.tile.y;

  // XY routing: change x until we are in the right column, then change y.
  if (xDest > xPos)      return EAST;
  else if (xDest < xPos) return WEST;
  else if (yDest > yPos) return SOUTH;
  else if (yDest < yPos) return NORTH;
  else                   return LOCAL;
}

void Router::reportStalls(ostream& os) {
  for (uint i=0; i<oData.length(); i++) {
    if (oData[i].valid()) {
      os << this->name() << ".output_" << DirectionNames[i] << " is blocked." << endl;
      os << "  Target destination is " << oData[i].read().channelID() << endl;
    }
  }

  for (uint i=0; i<iData.length(); i++) {
    if (inputBuffers[i].full()) {
      os << inputBuffers[i].name() << " is full." << endl;
      os << "  Head is trying to get to " << inputBuffers[i].peek().channelID() << endl;
    }
  }
}

Router::Router(const sc_module_name& name, const ComponentID& ID) :
    LokiComponent(name, ID),
    BlockingInterface(),
    inputBuffers(5, ROUTER_BUFFER_SIZE, string(this->name()) + ".input_data"),
    xPos(ID.tile.x),
    yPos(ID.tile.y) {

  state = WAITING_FOR_DATA;

  for (int i=0; i<5; i++) {
    lastAccepted[i] = -1;
    destination[i] = -1;
  }

  iData.init(5);    oData.init(5);
  iReady.init(5);   oReady.init(5);
  outputAvailable.init(5);

  // Generate a method to watch each input port, putting the data into the
  // appropriate buffer when it arrives.
  // Generate a method for each output port, sending data when there is data
  // to send.
  for (size_t i=0; i<iData.length(); i++) {
    SPAWN_METHOD(iData[i], Router::receiveData, i, false);
    SPAWN_METHOD(outputAvailable[i], Router::sendData, i, false);

    // Need to do this the long way because it's sensitive to multiple events.
    sc_core::sc_spawn_options options;
    options.spawn_method();     /* Want an efficient method, not a thread */
    options.set_sensitivity(&(inputBuffers[i].readEvent()));
    options.set_sensitivity(&(inputBuffers[i].writeEvent()));
    sc_spawn(sc_bind(&Router::updateFlowControl, this, i), 0, &options);
  }

}
