/*
 * WrappedTileComponent.cpp
 *
 *  Created on: 17 Feb 2010
 *      Author: db434
 */

#include "WrappedTileComponent.h"
#include "TileComponentFactory.h"

double WrappedTileComponent::area() const {
  return comp->area() + fcIn.area() + fcOut.area();
}

double WrappedTileComponent::energy() const {
  return comp->energy() + fcIn.energy() + fcOut.energy();
}

void WrappedTileComponent::storeData(std::vector<Word>& data) {
  comp->storeData(data);
}

void WrappedTileComponent::print(int start, int end) const {
  comp->print(start, end);
}

void WrappedTileComponent::initialise() {
  fcOut.initialise();
}

void WrappedTileComponent::setup() {

// Initialise arrays
  dataIn          = new sc_in<Word>[NUM_CLUSTER_INPUTS];
  requestsIn      = new sc_in<Word>[NUM_CLUSTER_INPUTS];
  responsesOut    = new sc_out<AddressedWord>[NUM_CLUSTER_INPUTS];
  dataOut         = new sc_out<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  requestsOut     = new sc_out<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  responsesIn     = new sc_in<Word>[NUM_CLUSTER_OUTPUTS];

  dataInSig       = new flag_signal<Word>[NUM_CLUSTER_INPUTS];
  dataOutSig      = new flag_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  dataOutSig2     = new flag_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  requestsOutSig  = new sc_buffer<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  fcOutSig        = new sc_signal<bool>[NUM_CLUSTER_OUTPUTS];
  fcInSig         = new sc_signal<int>[NUM_CLUSTER_INPUTS];

// Connect everything up
  comp->clock(clock);
  fcOut.clock(clock);
  comp->idle(idle);

  for(int i=0; i<NUM_CLUSTER_INPUTS; i++) {
    fcIn.dataIn[i](dataIn[i]);
    fcIn.requests[i](requestsIn[i]);
    fcIn.responses[i](responsesOut[i]);

    fcIn.flowControl[i](fcInSig[i]); comp->flowControlOut[i](fcInSig[i]);
    fcIn.dataOut[i](dataInSig[i]); comp->in[i](dataInSig[i]);
  }

  for(int i=0; i<NUM_CLUSTER_OUTPUTS; i++) {
    fcOut.dataOut[i](dataOutSig2[i]); dataOut[i](dataOutSig2[i]);
    fcOut.responses[i](responsesIn[i]);
    fcOut.requests[i](requestsOutSig[i]); requestsOut[i](requestsOutSig[i]);

    comp->flowControlIn[i](fcOutSig[i]); fcOut.flowControl[i](fcOutSig[i]);
    fcOut.dataIn[i](dataOutSig[i]); comp->out[i](dataOutSig[i]);
  }

}

WrappedTileComponent::WrappedTileComponent(sc_module_name name, int ID, int type) :
    Component(name, ID),
    fcIn("fc_in", NUM_CLUSTER_INPUTS),
    fcOut("fc_out", ID, NUM_CLUSTER_OUTPUTS) {

  comp = &(TileComponentFactory::makeTileComponent(type, ID));
  setup();
  end_module(); // Needed because we're using a different constructor

}

WrappedTileComponent::~WrappedTileComponent() {

}
