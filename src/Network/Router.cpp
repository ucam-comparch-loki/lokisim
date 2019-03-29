/*
 * Router.cpp
 *
 *  Created on: 27 Jun 2011
 *      Author: db434
 */

#include "Router.h"
#include "../Utility/Assert.h"
#include "../Utility/Instrumentation/Network.h"

// Some parts of SystemC don't seem to interact very well with templated
// classes. The best solution I've found is to list out the types which can
// be used.
template class Router<Word>;

const string DirectionNames[] = {"north", "east", "south", "west", "local"};

template<typename T>
void Router<T>::dataArrived() {
  loki_assert(iData.valid());

  if (!inputBuffers[LOCAL].canWrite())
    next_trigger(inputBuffers[LOCAL].canWriteEvent());
  else {
    inputBuffers[LOCAL].write(iData.read());
    iData.ack();
  }
}

template<typename T>
void Router<T>::updateFlowControl() {
  bool canReceive = inputBuffers[LOCAL].canWrite();
  if (oReady.read() != canReceive)
    oReady.write(canReceive);
}

template<typename T>
void Router<T>::sendData() {
  loki_assert(localOutput.dataAvailable());

  if (oData.valid())
    next_trigger(oData.ack_event());
  else if (iReady.read())
    oData.write(localOutput.read());
  else
    next_trigger(iReady.posedge_event());
}

template<typename T>
Router<T>::Router(const sc_module_name& name, const TileID& ID,
                  const router_parameters_t& params) :
    LokiComponent(name),
    clock("clock"),
    inputs("inputs", 4),
    outputs("outputs", 4),
    iData("iData"),
    oData("oData"),
    iReady("iReady"),
    oReady("oReady"),
    inputBuffers("in_buffers", 5, params.fifo.size),
    internal("network", ID),
    localOutput("to_local", 1) {

  internal.clock(clock);

  for (size_t i=0; i<inputs.size(); i++)
    inputs[i](inputBuffers[i]);

  for (uint i=0; i<inputBuffers.size(); i++)
    internal.inputs[i](inputBuffers[i]);

  for (uint i=0; i<outputs.size(); i++)
    internal.outputs[i](outputs[i]);

  // Temporary.
  internal.outputs[LOCAL](localOutput);

  SC_METHOD(updateFlowControl);
  sensitive << inputBuffers[LOCAL].canWriteEvent()
            << inputBuffers[LOCAL].dataAvailableEvent();
  // do initialise

  SC_METHOD(dataArrived);
  sensitive << iData;
  dont_initialize();

  SC_METHOD(sendData);
  sensitive << localOutput.dataAvailableEvent();
  dont_initialize();

}


template<typename T>
RouterInternalNetwork<T>::RouterInternalNetwork(const sc_module_name name, TileID tile) :
    Network2<T>(name, 5, 5),
    position(tile) {
  // Nothing
}

template<typename T>
RouterInternalNetwork<T>::~RouterInternalNetwork() {
  // Nothing
}

template<typename T>
PortIndex RouterInternalNetwork<T>::getDestination(const ChannelID address) const {
  TileID target = address.component.tile;

  // XY routing: change x until we are in the right column, then change y.
  if (target.x > position.x)      return EAST;
  else if (target.x < position.x) return WEST;
  else if (target.y > position.y) return SOUTH;
  else if (target.y < position.y) return NORTH;
  else                            return LOCAL;
}
