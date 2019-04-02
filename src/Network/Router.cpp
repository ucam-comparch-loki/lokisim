/*
 * Router.cpp
 *
 *  Created on: 27 Jun 2011
 *      Author: db434
 */

#include "Router.h"

// Some parts of SystemC don't seem to interact very well with templated
// classes. The best solution I've found is to list out the types which can
// be used.
template class Router<Word>;

const string DirectionNames[] = {"north", "east", "south", "west", "local"};

template<typename T>
Router<T>::Router(const sc_module_name& name, const TileID& ID,
                  const router_parameters_t& params) :
    LokiComponent(name),
    clock("clock"),
    inputs("inputs", 5),
    outputs("outputs", 5),
    inputBuffers("in_buffers", 5, params.fifo.size),
    internal("network", ID) {

  internal.clock(clock);

  for (size_t i=0; i<inputs.size(); i++)
    inputs[i](inputBuffers[i]);

  for (uint i=0; i<inputBuffers.size(); i++)
    internal.inputs[i](inputBuffers[i]);

  for (uint i=0; i<outputs.size(); i++)
    internal.outputs[i](outputs[i]);

}


template<typename T>
RouterInternalNetwork<T>::RouterInternalNetwork(const sc_module_name name, TileID tile) :
    Network<T>(name, 5, 5),
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
