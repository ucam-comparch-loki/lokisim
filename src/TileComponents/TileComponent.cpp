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
  ChannelID result = tile * INPUTS_PER_TILE;

  if(position >= CORES_PER_TILE) {
    assert(port <= NUM_MEMORY_INPUTS);
    result += CORES_PER_TILE * NUM_CORE_INPUTS;
    result += (position - CORES_PER_TILE) * NUM_MEMORY_INPUTS;
  }
  else {
    assert(port <= NUM_CORE_INPUTS);
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

/* Determine which component holds the given input port. */
ComponentID TileComponent::component(ChannelID port) {
  uint tile = port / INPUTS_PER_TILE;
  uint position = port % INPUTS_PER_TILE;
  ComponentID component = tile * COMPONENTS_PER_TILE;

  if(position >= CORES_PER_TILE*NUM_CORE_INPUTS) {
    component += CORES_PER_TILE;
    position  -= CORES_PER_TILE*NUM_CORE_INPUTS;
    component += position / NUM_MEMORY_INPUTS;
  }
  else {
    component += position / NUM_CORE_INPUTS;
  }
  return component;
}

/* Convert a global port ID into a string of the form "(component, port)". */
const std::string TileComponent::inputPortString(ChannelID port) {
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
const std::string TileComponent::outputPortString(ChannelID port) {
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
    Component(name, ID)/*,
    outBuffers("outputs", outputBuffers, outputPorts)*/ {

  flowControlOut = new sc_out<int>[inputPorts];
  in             = new sc_in<Word>[inputPorts];

  flowControlIn  = new sc_in<bool>[outputPorts];
  out            = new sc_out<AddressedWord>[outputPorts];

//  toOutputBuffers = new sc_buffer<AddressedWord>[outputBuffers];
//  bufferAvailable = new sc_buffer<bool>[outputBuffers];
//  emptyOutputBuffer = new sc_buffer<bool>[outputBuffers];

//  // Connect things up.
//  outBuffers.clock(clock);
//
//  for(int i=0; i<outputPorts; i++) {
//    outBuffers.dataOut[i](out[i]);
//    outBuffers.creditsIn[i](flowControlIn[i]);
//  }
//
//  for(int i=0; i<outputBuffers; i++) {
//    outBuffers.dataIn[i](toOutputBuffers[i]);
//    outBuffers.emptyBuffer[i](emptyOutputBuffer[i]);
//    outBuffers.flowControlOut[i](bufferAvailable[i]);
//  }

  idle.initialize(true);

}

TileComponent::~TileComponent() {
  delete[] in;
  delete[] out;
  delete[] flowControlIn;
  delete[] flowControlOut;

//  delete[] toOutputBuffers;
//  delete[] emptyOutputBuffer;
//  delete[] bufferAvailable;
}
