/*
 * WrappedTileComponent.cpp
 *
 *  Created on: 17 Feb 2010
 *      Author: db434
 */

#include "WrappedTileComponent.h"
#include "TileComponentFactory.h"

void WrappedTileComponent::setup() {

// Initialise arrays
  dataIn          = new sc_in<Word>[NUM_CLUSTER_INPUTS];
  requestsIn      = new sc_in<Word>[NUM_CLUSTER_INPUTS];
  responsesOut    = new sc_out<AddressedWord>[NUM_CLUSTER_INPUTS];
  dataOut         = new sc_out<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  requestsOut     = new sc_out<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  responsesIn     = new sc_in<Word>[NUM_CLUSTER_OUTPUTS];

  dataInSig       = new sc_signal<Word>[NUM_CLUSTER_INPUTS];
  dataOutSig      = new sc_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  dataOutSig2     = new sc_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  requestsOutSig  = new sc_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  fcOutSig        = new sc_signal<bool>[NUM_CLUSTER_OUTPUTS];
  fcInSig         = new sc_signal<bool>[NUM_CLUSTER_INPUTS];

// Connect everything up
  comp->clock(clock);

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

/* Deprecate/remove this constructor? */
WrappedTileComponent::WrappedTileComponent(sc_module_name name, TileComponent& tc) :
    Component(name, tc.id),
    fcIn("fc_in", NUM_CLUSTER_INPUTS),
    fcOut("fc_out", NUM_CLUSTER_OUTPUTS) {

  comp = &tc;
  setup();
  end_module(); // Needed because we're using a different constructor

}

WrappedTileComponent::WrappedTileComponent(sc_module_name name, int ID, int type) :
    Component(name, ID),
    fcIn("fc_in", NUM_CLUSTER_INPUTS),
    fcOut("fc_out", NUM_CLUSTER_OUTPUTS) {

  comp = &(TileComponentFactory::makeTileComponent(type, ID));
  setup();
  end_module(); // Needed because we're using a different constructor

}

WrappedTileComponent::~WrappedTileComponent() {

}
