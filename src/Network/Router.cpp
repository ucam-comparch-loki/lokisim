/*
 * Router.cpp
 *
 *  Created on: 27 Jun 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Router.h"
#include "../Utility/Instrumentation/Network.h"

const string DirectionNames[] = {"north", "east", "south", "west", "local"};

void Router::receiveData(PortIndex input) {
  if (inputBuffers[input].full()) {
    // If the buffer is full, let the data sit on the wire until space becomes
    // available.
    // TODO: remove this, and rely on the flow control signals
    next_trigger(inputBuffers[input].readEvent());
  }
  else {
    Instrumentation::Network::recordBandwidth(iData[input].name());
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
    // Wormhole mode - continue reading from same input.
    if (holdInput[output]) {
      PortIndex input = lastAccepted[output];
      if (!inputBuffers[input].empty()) {
        NetworkData flit = inputBuffers[input].read();
        oData[output].write(flit);

        Instrumentation::Network::recordBandwidth(oData[output].name());

        // Stick with this input if necessary.
        holdInput[output] = wormhole && !flit.getMetadata().endOfPacket;

        next_trigger(oData[output].ack_event());
        updateDestination(input);
        return;
      }
    }
    // Find new packet to send.
    else {
      for (int i=0; i<5; i++) {
        PortIndex input = (i + lastAccepted[output] + 1) % 5;
        if (destination[input] == output) {
          LOKI_LOG << this->name() << " sending to " << DirectionNames[output] << ": " << inputBuffers[input].peek() << endl;
          Instrumentation::Network::recordBandwidth(oData[output].name());

          NetworkData flit = inputBuffers[input].read();
          oData[output].write(flit);
          lastAccepted[output] = input;

          // Stick with this input if necessary.
          holdInput[output] = wormhole && !flit.getMetadata().endOfPacket;

          next_trigger(oData[output].ack_event());
          updateDestination(input);
          return;
        }
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
    output = getDestination(inputBuffers[input].peek().channelID());
    outputAvailable[output].notify();
  }

  destination[input] = output;
}

PortIndex Router::getDestination(ChannelID address) const {
  TileID target = address.component.tile;

  // XY routing: change x until we are in the right column, then change y.
  if (target.x > position.x)      return EAST;
  else if (target.x < position.x) return WEST;
  else if (target.y > position.y) return SOUTH;
  else if (target.y < position.y) return NORTH;
  else                            return LOCAL;
}

void Router::reportStalls(ostream& os) {
  for (uint i=0; i<oData.size(); i++) {
    if (oData[i].valid()) {
      os << this->name() << ".output_" << DirectionNames[i] << " is blocked." << endl;
      os << "  Target destination is " << oData[i].read().channelID() << endl;
    }
  }

  for (uint i=0; i<iData.size(); i++) {
    if (inputBuffers[i].full()) {
      os << inputBuffers[i].name() << " is full." << endl;
      os << "  Head is trying to get to " << inputBuffers[i].peek().channelID() << endl;
    }
  }
}

Router::Router(const sc_module_name& name, const TileID& ID,
               const router_parameters_t& params) :
    Network(name, 5, 5),
    BlockingInterface(),
    iData("iData", 5),
    oReady("oReady", 5),
    oData("oData", 5),
    iReady("iReady", 5),
    inputBuffers(string(this->name()) + ".input_data", 5, params.fifo.size),
    position(ID),
    wormhole(true),
    outputAvailable("outputAvailableEvent", 5) {

  state = WAITING_FOR_DATA;

  for (int i=0; i<5; i++) {
    lastAccepted[i] = -1;
    destination[i] = -1;
    holdInput[i] = false;
  }

  // Generate a method to watch each input port, putting the data into the
  // appropriate buffer when it arrives.
  // Generate a method for each output port, sending data when there is data
  // to send.
  for (size_t i=0; i<iData.size(); i++) {
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
