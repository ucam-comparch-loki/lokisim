/*
 * Tile.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Tile.h"

void Tile::storeData(vector<Word>& data, int componentNumber) {
  contents[componentNumber]->storeData(data);
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

  int numOutputs = NUM_CLUSTER_OUTPUTS * COMPONENTS_PER_TILE;
  int numInputs  = NUM_CLUSTER_INPUTS  * COMPONENTS_PER_TILE;

  responsesToCluster   = new sc_buffer<Word>[numOutputs];
  responsesFromCluster = new sc_buffer<AddressedWord>[numInputs];
  requestsToCluster    = new sc_buffer<Word>[numInputs];
  dataToCluster        = new flag_signal<Word>[numInputs];

  // Initialise the Clusters of this Tile
  for(int i=0; i<CLUSTERS_PER_TILE; i++) {
    contents.push_back(new WrappedTileComponent("wrapped", i, TileComponent::CLUSTER));
  }

  // Initialise the memories of this Tile
  for(int i=CLUSTERS_PER_TILE; i<COMPONENTS_PER_TILE; i++) {
    contents.push_back(new WrappedTileComponent("wrapped", i, TileComponent::MEMORY));
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

}

Tile::~Tile() {

}
