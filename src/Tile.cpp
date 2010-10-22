/*
 * Tile.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Tile.h"
#include "TileComponents/TileComponent.h"
#include "TileComponents/WrappedTileComponent.h"
#include "Datatype/Address.h"

double Tile::area() const {
  // Update this if allowing heterogeneity.
  return network.area() +
         contents[0]->area() * CLUSTERS_PER_TILE +
         contents[CLUSTERS_PER_TILE]->area() * MEMS_PER_TILE;
}

double Tile::energy() const {
  // Update this if allowing heterogeneity.
  return network.energy() +
         contents[0]->energy() * CLUSTERS_PER_TILE +
         contents[CLUSTERS_PER_TILE]->energy() * MEMS_PER_TILE;
}

bool Tile::isIdle() const {
  return idleVal;
}

void Tile::storeData(vector<Word>& data, int componentNumber, int location) {
  contents[componentNumber]->storeData(data, location);
}

void Tile::print(int component, int start, int end) const {
  if(start<end) contents[component]->print(start, end);
  else          contents[component]->print(end, start);
}

Word Tile::getMemVal(int component, int addr) const {
  return contents[component]->getMemVal(addr);
}

int Tile::getRegVal(int component, int reg) const {
  return contents[component]->getRegVal(reg);
}

Address Tile::getInstIndex(int component) const {
  return contents[component]->getInstIndex();
}

bool Tile::getPredReg(int component) const {
  return contents[component]->getPredReg();
}

void Tile::updateIdle() {
  idleVal = true;

  for(uint i=0; i<COMPONENTS_PER_TILE; i++) {
    idleVal &= idleSig[i].read();
  }

  // Also check network to make sure data/instructions aren't in transit

  idle.write(idleVal);
}

Tile::Tile(sc_module_name name, int ID) :
    Component(name, ID),
    network("localnetwork") {

  idleSig = new sc_signal<bool>[COMPONENTS_PER_TILE];

  SC_METHOD(updateIdle);
  for(uint i=0; i<COMPONENTS_PER_TILE; i++) sensitive << idleSig[i];
  // do initialise

  int numOutputs = NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE;
  int numInputs  = NUM_CLUSTER_INPUTS  * COMPONENTS_PER_TILE;

  creditsToCluster   = new flag_signal<AddressedWord>[numOutputs];
  creditsFromCluster = new flag_signal<AddressedWord>[numInputs];
  dataToCluster        = new flag_signal<AddressedWord>[numInputs];

  network.clock(clock);

  // Initialise the Clusters of this Tile
  for(uint i=0; i<CLUSTERS_PER_TILE; i++) {
    WrappedTileComponent* wtc = new WrappedTileComponent("wrapped", i, TileComponent::CLUSTER);
    wtc->idle(idleSig[i]);
    contents.push_back(wtc);
  }

  // Initialise the memories of this Tile
  for(uint i=CLUSTERS_PER_TILE; i<COMPONENTS_PER_TILE; i++) {
    WrappedTileComponent* wtc = new WrappedTileComponent("wrapped", i, TileComponent::MEMORY);
    wtc->idle(idleSig[i]);
    contents.push_back(wtc);
  }

  // Connect the clusters and memories to the local interconnect
  for(uint i=0; i<COMPONENTS_PER_TILE; i++) {

    for(uint j=0; j<NUM_CLUSTER_INPUTS; j++) {
      int index = i*NUM_CLUSTER_INPUTS + j;   // Position in network's array

      contents[i]->dataIn[j](dataToCluster[index]);
      network.dataOut[index](dataToCluster[index]);
      contents[i]->creditsOut[j](creditsFromCluster[index]);
      network.creditsIn[index](creditsFromCluster[index]);
    }

    for(uint j=0; j<NUM_CLUSTER_OUTPUTS; j++) {
      int index = i*NUM_CLUSTER_OUTPUTS + j;  // Position in network's array

      network.dataIn[index](contents[i]->dataOut[j]);
      network.creditsOut[index](creditsToCluster[index]);
      contents[i]->creditsIn[j](creditsToCluster[index]);
    }

    contents[i]->clock(clock);
    contents[i]->initialise();

  }

  end_module(); // Needed because we're using a different Component constructor

}

Tile::~Tile() {
  delete[] idleSig;
  delete[] creditsToCluster;
  delete[] dataToCluster;
  delete[] creditsFromCluster;

  for(uint i=0; i<contents.size(); i++) delete contents[i];
}
