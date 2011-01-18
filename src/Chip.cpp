/*
 * Chip.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Chip.h"
#include "TileComponents/Cluster.h"
#include "TileComponents/MemoryMat.h"
#include "Datatype/Address.h"

double Chip::area() const {
  // Update this if allowing heterogeneity.
  return network.area() +
         contents[0]->area() * CORES_PER_TILE +
         contents[CORES_PER_TILE]->area() * MEMS_PER_TILE;
}

double Chip::energy() const {
  // Update this if allowing heterogeneity.
  return network.energy() +
         contents[0]->energy() * CORES_PER_TILE +
         contents[CORES_PER_TILE]->energy() * MEMS_PER_TILE;
}

bool Chip::isIdle() const {
  return idleVal;
}

void Chip::storeData(vector<Word>& data, ComponentID component, MemoryAddr location) {
  contents[component]->storeData(data, location);
}

void Chip::print(ComponentID component, MemoryAddr start, MemoryAddr end) const {
  if(start<end) contents[component]->print(start, end);
  else          contents[component]->print(end, start);
}

Word Chip::getMemVal(ComponentID component, MemoryAddr addr) const {
  return contents[component]->getMemVal(addr);
}

int Chip::getRegVal(ComponentID component, RegisterIndex reg) const {
  return contents[component]->readRegDebug(reg);
}

Address Chip::getInstIndex(ComponentID component) const {
  return contents[component]->getInstIndex();
}

bool Chip::getPredReg(ComponentID component) const {
  return contents[component]->readPredReg();
}

void Chip::updateIdle() {
  idleVal = true;

  for(uint i=0; i<COMPONENTS_PER_TILE; i++) {
    idleVal &= idleSig[i].read();
  }

  // Also check network to make sure data/instructions aren't in transit

  idle.write(idleVal);
}

Chip::Chip(sc_module_name name, ComponentID ID) :
    Component(name),
    network("network") {

  idleSig = new sc_signal<bool>[NUM_COMPONENTS];

  SC_METHOD(updateIdle);
  for(uint i=0; i<NUM_COMPONENTS; i++) sensitive << idleSig[i];
  // do initialise

  int numOutputs = TOTAL_OUTPUTS;
  int numInputs  = TOTAL_INPUTS;

  dataFromComponents = new flag_signal<AddressedWord>[numOutputs];
  dataToComponents = new flag_signal<Word>[numInputs];
  creditsFromComponents = new flag_signal<int>[numInputs];
  readyToComponents = new sc_signal<bool>[numOutputs];

  network.clock(clock);

  for(uint j=0; j<NUM_TILES; j++) {
    // Initialise the Clusters of this Tile
    for(uint i=0; i<CORES_PER_TILE; i++) {
      ComponentID clusterID = j*COMPONENTS_PER_TILE + i;
      Cluster* c = new Cluster("cluster", clusterID);
      c->idle(idleSig[clusterID]);
      contents.push_back(c);
    }

    // Initialise the memories of this Tile
    for(uint i=CORES_PER_TILE; i<COMPONENTS_PER_TILE; i++) {
      ComponentID memoryID = j*COMPONENTS_PER_TILE + i;
      MemoryMat* m = new MemoryMat("memory", memoryID);
      m->idle(idleSig[memoryID]);
      contents.push_back(m);
    }
  }

  // Connect the clusters and memories to the local interconnect
  for(uint i=0; i<NUM_COMPONENTS; i++) {
    bool isCore = (i%COMPONENTS_PER_TILE) < CORES_PER_TILE;
    int numInputs = isCore ? NUM_CORE_INPUTS : NUM_MEMORY_INPUTS;
    int numOutputs = isCore ? NUM_CORE_OUTPUTS : NUM_MEMORY_OUTPUTS;

    for(int j=0; j<numInputs; j++) {
      int index = TileComponent::inputPortID(i,j);  // Position in network's array

      contents[i]->in[j](dataToComponents[index]);
      network.dataOut[index](dataToComponents[index]);
      contents[i]->flowControlOut[j](creditsFromComponents[index]);
      network.creditsIn[index](creditsFromComponents[index]);
    }

    for(int j=0; j<numOutputs; j++) {
      int index = TileComponent::outputPortID(i,j); // Position in network's array

      contents[i]->out[j](dataFromComponents[index]);
      network.dataIn[index](dataFromComponents[index]);
      contents[i]->flowControlIn[j](readyToComponents[index]);
      network.readyOut[index](readyToComponents[index]);
    }

    contents[i]->clock(clock);

  }

}

Chip::~Chip() {
  delete[] idleSig;
  delete[] dataFromComponents;
  delete[] dataToComponents;
  delete[] creditsFromComponents;
  delete[] readyToComponents;

  for(uint i=0; i<contents.size(); i++) delete contents[i];
}
