/*
 * WrappedTileComponent.cpp
 *
 *  Created on: 17 Feb 2010
 *      Author: db434
 */

#include "WrappedTileComponent.h"

WrappedTileComponent::WrappedTileComponent(sc_core::sc_module_name name, int ID,
                                           TileComponent tc) :
    Component(name, ID),
    comp(tc),
    fcIn("fc in", ID, NUM_CLUSTER_INPUTS),
    fcOut("fc out", ID, NUM_CLUSTER_OUTPUTS) {

// Initialise arrays
  dataIn = new sc_in<Word>[NUM_CLUSTER_INPUTS];
  requestsIn = new sc_in<Word>[NUM_CLUSTER_INPUTS];
  responsesIn = new sc_in<Word>[NUM_CLUSTER_INPUTS];
  dataOut = new sc_out<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  requestsOut = new sc_out<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  responsesOut = new sc_out<AddressedWord>[NUM_CLUSTER_OUTPUTS];

  dataToComponent = new sc_signal<Word>[NUM_CLUSTER_INPUTS];
  dataFromComponent = new sc_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];

// Connect everything up
  comp.clock(clock);

  for(int i=0; i<NUM_CLUSTER_INPUTS; i++) {
    fcIn.dataIn[i](dataIn[i]);
    fcIn.requests[i](requestsIn[i]);
    fcIn.responses[i](responsesOut[i]);

    fcIn.flowControl[i](comp.flowControlIn[i]);
    fcIn.dataOut[i](dataToComponent[i]);
    comp.in[i](dataToComponent[i]);
  }

  for(int i=0; i<NUM_CLUSTER_OUTPUTS; i++) {
    fcOut.dataOut[i](dataOut[i]);
    fcOut.requests[i](requestsOut[i]);
    fcOut.responses[i](responsesIn[i]);

    fcOut.flowControl[i](comp.flowControlOut[i]);
    fcOut.dataIn[i](dataFromComponent[i]);
    comp.out[i](dataFromComponent[i]);
  }

}

WrappedTileComponent::~WrappedTileComponent() {

}
