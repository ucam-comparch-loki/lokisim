/*
 * Tile.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Tile.h"

Tile* Tile::currentTile = 0;

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

void Tile::storeData(vector<Word>& data, int componentNumber) {
  contents[componentNumber]->storeData(data);
}

void Tile::print(int component, int start, int end, Tile* tile) {
  if(start<end) tile->contents[component]->print(start, end);
  else          tile->contents[component]->print(end, start);
}

int Tile::getRegVal(int component, int reg, Tile* tile) {
  return tile->contents[component]->getRegVal(reg);
}

/* Connect two horizontally-adjacent Tiles together. */
void Tile::connectLeftRight(const Tile& left, const Tile& right) {
  for(int i=0; i<NUM_CHANNELS_BETWEEN_TILES; i++) {
//    right.inWest[i](left.outEast[i]);
//    left.inEast[i](right.outWest[i]);
  }
}

/* Connect two vertically-adjacent Tiles together. */
void Tile::connectTopBottom(const Tile& top, const Tile& bottom) {
  for(int i=0; i<NUM_CHANNELS_BETWEEN_TILES; i++) {
//    bottom.inNorth[i](top.outSouth[i]);
//    top.inSouth[i](bottom.outNorth[i]);
  }
}

void Tile::updateIdle() {
  bool isIdle = true;

  for(int i=0; i<COMPONENTS_PER_TILE; i++) {
    isIdle &= idleSig[i].read();
  }

  // Also check network to make sure data/instructions aren't in transit

  idle.write(isIdle);
}

Tile::Tile(sc_module_name name, int ID) :
    Component(name, ID),
    network("localnetwork") {

  // Initialise arrays of inputs and outputs
//  inNorth  = new sc_in<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//  inEast   = new sc_in<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//  inSouth  = new sc_in<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//  inWest   = new sc_in<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//  outNorth = new sc_out<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//  outEast  = new sc_out<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//  outSouth = new sc_out<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];
//  outWest  = new sc_out<AddressedWord>[NUM_CHANNELS_BETWEEN_TILES];

  idleSig = new sc_signal<bool>[COMPONENTS_PER_TILE];

  SC_METHOD(updateIdle);
  for(int i=0; i<COMPONENTS_PER_TILE; i++) sensitive << idleSig[i];
  // do initialise

  int numOutputs = NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE;
  int numInputs  = NUM_CLUSTER_INPUTS  * COMPONENTS_PER_TILE;

  responsesToCluster   = new flag_signal<Word>[numOutputs];
  responsesFromCluster = new flag_signal<AddressedWord>[numInputs];
  requestsToCluster    = new flag_signal<Word>[numInputs];
  dataToCluster        = new flag_signal<Word>[numInputs];

  network.clock(clock);

  // Initialise the Clusters of this Tile
  for(int i=0; i<CLUSTERS_PER_TILE; i++) {
    WrappedTileComponent* wtc = new WrappedTileComponent("wrapped", i, TileComponent::CLUSTER);
    wtc->idle(idleSig[i]);
    contents.push_back(wtc);
  }

  // Initialise the memories of this Tile
  for(int i=CLUSTERS_PER_TILE; i<COMPONENTS_PER_TILE; i++) {
    WrappedTileComponent* wtc = new WrappedTileComponent("wrapped", i, TileComponent::MEMORY);
    wtc->idle(idleSig[i]);
    contents.push_back(wtc);
  }

  // Connect the clusters and memories to the local interconnect
  for(int i=0; i<COMPONENTS_PER_TILE; i++) {

    for(int j=0; j<NUM_CLUSTER_INPUTS; j++) {
      int index = i*NUM_CLUSTER_INPUTS + j;   // Position in network's array

      contents[i]->dataIn[j](dataToCluster[index]);
      network.dataOut[index](dataToCluster[index]);
      contents[i]->requestsIn[j](requestsToCluster[index]);
      network.requestsOut[index](requestsToCluster[index]);
      contents[i]->responsesOut[j](responsesFromCluster[index]);
      network.responsesIn[index](responsesFromCluster[index]);
    }

    for(int j=0; j<NUM_CLUSTER_OUTPUTS; j++) {
      int index = i*NUM_CLUSTER_OUTPUTS + j;  // Position in network's array

      network.dataIn[index](contents[i]->dataOut[j]);
      network.requestsIn[index](contents[i]->requestsOut[j]);
      network.responsesOut[index](responsesToCluster[index]);
      contents[i]->responsesIn[j](responsesToCluster[index]);
    }

    contents[i]->clock(clock);
    contents[i]->initialise();

  }

  end_module(); // Needed because we're using a different Component constructor

  currentTile = this;

}

Tile::~Tile() {

}
