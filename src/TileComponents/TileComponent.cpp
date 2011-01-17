/*
 * TileComponent.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "TileComponent.h"
#include "../Datatype/Address.h"
#include "../Datatype/AddressedWord.h"

void TileComponent::print(MemoryAddr start, MemoryAddr end) const {
  // Do nothing if print isn't defined
}

Word TileComponent::getMemVal(MemoryAddr addr) const {
  return Word(-1);
}

int32_t TileComponent::readRegDebug(RegisterIndex reg) const {
  return -1;
}

Address TileComponent::getInstIndex() const {
  return Address(-1, -1);
}

bool TileComponent::readPredReg() const {
  return false;
}

/* Generate a global port ID using the component and the index of the port
 * on that component. */
ChannelID TileComponent::inputPortID(ComponentID component, ChannelIndex port) {
  uint tile = component / COMPONENTS_PER_TILE;
  uint position = component % COMPONENTS_PER_TILE;
  ChannelID result = tile * INPUTS_PER_TILE;

  if(position >= CORES_PER_TILE) {
    result += CORES_PER_TILE * NUM_CORE_INPUTS;
    result += (position - CORES_PER_TILE) * NUM_MEMORY_INPUTS;
  }
  else {
    result += position * NUM_CORE_INPUTS;
  }

  return result + port;
}

/* Generate a global port ID using the component and the index of the port
 * on that component. */
ChannelID TileComponent::outputPortID(ComponentID component, ChannelIndex port) {
  uint tile = component / COMPONENTS_PER_TILE;
  uint position = component % COMPONENTS_PER_TILE;
  ChannelID result = tile * OUTPUTS_PER_TILE;

  if(position >= CORES_PER_TILE) {
    result += CORES_PER_TILE * NUM_CORE_OUTPUTS;
    result += (position - CORES_PER_TILE) * NUM_MEMORY_OUTPUTS;
  }
  else {
    result += position * NUM_CORE_OUTPUTS;
  }

  return result + port;
}

/* Convert a global port ID into a string of the form "(component, port)". */
std::string TileComponent::inputPortString(ChannelID port) {
  uint tile = port / INPUTS_PER_TILE;
  uint position = port % INPUTS_PER_TILE;
  ComponentID component = tile * COMPONENTS_PER_TILE;
  ChannelIndex portIndex;

  if(position >= CORES_PER_TILE*NUM_CORE_INPUTS) {
    component += CORES_PER_TILE;
    position  -= CORES_PER_TILE*NUM_CORE_INPUTS;
    component += position / NUM_MEMORY_INPUTS;
    portIndex  = position % NUM_MEMORY_INPUTS;
  }
  else {
    component += position / NUM_CORE_INPUTS;
    portIndex  = position % NUM_CORE_INPUTS;
  }

  std::stringstream ss;
  ss << "(" << component << "," << (int)portIndex << ")";
  std::string result;
  ss >> result;
  return result;
}

/* Convert a global port ID into a string of the form "(component, port)". */
std::string TileComponent::outputPortString(ChannelID port) {
  uint tile = port / OUTPUTS_PER_TILE;
  uint position = port % OUTPUTS_PER_TILE;
  ComponentID component = tile * COMPONENTS_PER_TILE;
  ChannelIndex portIndex;

  if(position >= CORES_PER_TILE*NUM_CORE_OUTPUTS) {
    component += CORES_PER_TILE;
    position  -= CORES_PER_TILE*NUM_CORE_OUTPUTS;
    component += position / NUM_MEMORY_OUTPUTS;
    portIndex  = position % NUM_MEMORY_OUTPUTS;
  }
  else {
    component += position / NUM_CORE_OUTPUTS;
    portIndex  = position % NUM_CORE_OUTPUTS;
  }

  std::stringstream ss;
  ss << "(" << component << "," << (int)portIndex << ")";
  std::string result;
  ss >> result;
  return result;
}

/* Constructors and destructors */
TileComponent::TileComponent(sc_module_name name, ComponentID ID) :
    Component(name, ID) {

  idle.initialize(true);
  Instrumentation::idle(id, true);

}

TileComponent::~TileComponent() {
  delete[] in;
  delete[] out;
  delete[] flowControlIn;
  delete[] flowControlOut;
}
