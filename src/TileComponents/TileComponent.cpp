/*
 * TileComponent.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "TileComponent.h"
#include "../Chip.h"
#include "../Datatype/AddressedWord.h"

void TileComponent::print(MemoryAddr start, MemoryAddr end) const {
  // Do nothing if print isn't defined
}

const Word TileComponent::readWord(MemoryAddr addr) const {return Word(-1);}
const Word TileComponent::readByte(MemoryAddr addr) const {return Word(-1);}
void TileComponent::writeWord(MemoryAddr addr, Word data) {}
void TileComponent::writeByte(MemoryAddr addr, Word data) {}

const int32_t TileComponent::readRegDebug(RegisterIndex reg) const {
  return -1;
}

const MemoryAddr TileComponent::getInstIndex() const {
  return 0xFFFFFFFF;
}

bool TileComponent::readPredReg() const {
  return false;
}

/* Generate a global port ID using the component and the index of the port
 * on that component. */
ChannelID TileComponent::inputPortID(ComponentID component, ChannelIndex port) {
  uint tile = component / COMPONENTS_PER_TILE;
  uint position = component % COMPONENTS_PER_TILE;
  ChannelID result = tile * INPUT_PORTS_PER_TILE;

  if(position >= CORES_PER_TILE) {
    assert(port < MEMORY_INPUT_PORTS);
    result += CORES_PER_TILE * CORE_INPUT_PORTS;
    result += (position - CORES_PER_TILE) * MEMORY_INPUT_PORTS;
  }
  else {
    assert(port < CORE_INPUT_PORTS);
    result += position * CORE_INPUT_PORTS;
  }

  return result + port;
}

/* Generate a global port ID using the component and the index of the port
 * on that component. */
ChannelID TileComponent::outputPortID(ComponentID component, PortIndex port) {
  uint tile = component / COMPONENTS_PER_TILE;
  uint position = component % COMPONENTS_PER_TILE;
  ChannelID result = tile * OUTPUT_PORTS_PER_TILE;

  if(position >= CORES_PER_TILE) {
    assert(port < MEMORY_OUTPUT_PORTS);
    result += CORES_PER_TILE * CORE_OUTPUT_PORTS;
    result += (position - CORES_PER_TILE) * MEMORY_OUTPUT_PORTS;
  }
  else {
    assert(port < CORE_OUTPUT_PORTS);
    result += position * CORE_OUTPUT_PORTS;
  }

  return result + port;
}

/* Generate a global channel ID using the component and the index of the channel
 * on that component. */
ChannelID TileComponent::inputChannelID(ComponentID component, ChannelIndex channel) {
  uint tile = component / COMPONENTS_PER_TILE;
  uint position = component % COMPONENTS_PER_TILE;
  ChannelID result = tile * INPUT_CHANNELS_PER_TILE;

  if(position >= CORES_PER_TILE) {
    assert(channel < MEMORY_INPUT_CHANNELS);
    result += CORES_PER_TILE * CORE_INPUT_CHANNELS;
    result += (position - CORES_PER_TILE) * MEMORY_INPUT_CHANNELS;
  }
  else {
    assert(channel < CORE_INPUT_CHANNELS);
    result += position * CORE_INPUT_CHANNELS;
  }

  return result + channel;
}

/* Generate a global channel ID using the component and the index of the channel
 * on that component. */
ChannelID TileComponent::outputChannelID(ComponentID component, ChannelIndex channel) {
  uint tile = component / COMPONENTS_PER_TILE;
  uint position = component % COMPONENTS_PER_TILE;
  ChannelID result = tile * OUTPUT_CHANNELS_PER_TILE;

  if(position >= CORES_PER_TILE) {
    assert(channel < MEMORY_OUTPUT_CHANNELS);
    result += CORES_PER_TILE * CORE_OUTPUT_CHANNELS;
    result += (position - CORES_PER_TILE) * MEMORY_OUTPUT_CHANNELS;
  }
  else {
    assert(channel < CORE_OUTPUT_CHANNELS);
    result += position * CORE_OUTPUT_CHANNELS;
  }

  return result + channel;
}

/* Determine which component holds the given input channel. */
ComponentID TileComponent::component(ChannelID channel) {
  uint tile = channel / INPUT_CHANNELS_PER_TILE;
  uint position = channel % INPUT_CHANNELS_PER_TILE;
  ComponentID component = tile * COMPONENTS_PER_TILE;

  if(position >= CORES_PER_TILE*CORE_INPUT_CHANNELS) {
    component += CORES_PER_TILE;
    position  -= CORES_PER_TILE*CORE_INPUT_CHANNELS;
    component += position / MEMORY_INPUT_CHANNELS;
  }
  else {
    component += position / CORE_INPUT_CHANNELS;
  }
  return component;
}

/* Convert a global channel ID into a string of the form "(component, channel)". */
const std::string TileComponent::inputPortString(ChannelID channel) {
  uint tile = channel / INPUT_PORTS_PER_TILE;
  uint position = channel % INPUT_PORTS_PER_TILE;
  ComponentID component = tile * COMPONENTS_PER_TILE;
  ChannelIndex channelIndex;

  if(position >= CORES_PER_TILE*CORE_INPUT_PORTS) {
    component += CORES_PER_TILE;
    position  -= CORES_PER_TILE*CORE_INPUT_PORTS;
    component += position / MEMORY_INPUT_PORTS;
    channelIndex  = position % MEMORY_INPUT_PORTS;
  }
  else {
    component += position / CORE_INPUT_PORTS;
    channelIndex  = position % CORE_INPUT_PORTS;
  }

  std::stringstream ss;
  ss << "(" << component << "," << (int)channelIndex << ")";
  std::string result;
  ss >> result;
  return result;
}

/* Convert a global channel ID into a string of the form "(component, channel)". */
const std::string TileComponent::outputPortString(ChannelID channel) {
  uint tile = channel / OUTPUT_CHANNELS_PER_TILE;
  uint position = channel % OUTPUT_CHANNELS_PER_TILE;
  ComponentID component = tile * COMPONENTS_PER_TILE;
  ChannelIndex channelIndex;

  if(position >= CORES_PER_TILE*CORE_OUTPUT_CHANNELS) {
    component += CORES_PER_TILE;
    position  -= CORES_PER_TILE*CORE_OUTPUT_CHANNELS;
    component += position / MEMORY_OUTPUT_CHANNELS;
    channelIndex  = position % MEMORY_OUTPUT_CHANNELS;
  }
  else {
    component += position / CORE_OUTPUT_CHANNELS;
    channelIndex  = position % CORE_OUTPUT_CHANNELS;
  }

  std::stringstream ss;
  ss << "(" << component << "," << (int)channelIndex << ")";
  std::string result;
  ss >> result;
  return result;
}

int32_t TileComponent::readMemWord(MemoryAddr addr) const {
  // Assuming we want the first memory on this tile, and that its ID comes
  // after all the cores.
  int tile = id / COMPONENTS_PER_TILE;
  ComponentID memory = tile*COMPONENTS_PER_TILE + CORES_PER_TILE;

  return parent()->readWord(memory, addr).toInt();
}

int32_t TileComponent::readMemByte(MemoryAddr addr) const {
  // Assuming we want the first memory on this tile, and that its ID comes
  // after all the cores.
  int tile = id / COMPONENTS_PER_TILE;
  ComponentID memory = tile*COMPONENTS_PER_TILE + CORES_PER_TILE;

  return parent()->readByte(memory, addr).toInt();
}

void TileComponent::writeMemWord(MemoryAddr addr, Word data) const {
  // Assuming we want the first memory on this tile, and that its ID comes
  // after all the cores.
  int tile = id / COMPONENTS_PER_TILE;
  ComponentID memory = tile*COMPONENTS_PER_TILE + CORES_PER_TILE;

  parent()->writeWord(memory, addr, data);
}

void TileComponent::writeMemByte(MemoryAddr addr, Word data) const {
  // Assuming we want the first memory on this tile, and that its ID comes
  // after all the cores.
  int tile = id / COMPONENTS_PER_TILE;
  ComponentID memory = tile*COMPONENTS_PER_TILE + CORES_PER_TILE;

  parent()->writeByte(memory, addr, data);
}

Chip* TileComponent::parent() const {
  return static_cast<Chip*>(this->get_parent());
}

/* Constructors and destructors */
TileComponent::TileComponent(sc_module_name name, ComponentID ID,
                             int inputPorts, int outputPorts) :
    Component(name, ID) {

  dataIn               = new sc_in<AddressedWord>[inputPorts];
  canReceiveData   = new sc_out<bool>[inputPorts];

  dataOut              = new sc_out<AddressedWord>[outputPorts];
  canSendData      = new sc_in<bool>[outputPorts];

  creditsOut       = new sc_out<AddressedWord>[inputPorts];
  canSendCredit    = new sc_in<bool>[inputPorts];

  creditsIn        = new sc_in<AddressedWord>[outputPorts];
  canReceiveCredit = new sc_out<bool>[outputPorts];

  idle.initialize(true);
  for(int i=0; i<outputPorts; i++) canReceiveCredit[i].initialize(true);
  for(int i=0; i<inputPorts; i++)  canReceiveData[i].initialize(true);

}

TileComponent::~TileComponent() {
  delete[] dataIn;
  delete[] dataOut;
  delete[] canSendData;
  delete[] creditsOut;
  delete[] creditsIn;
  delete[] canReceiveCredit;
}
