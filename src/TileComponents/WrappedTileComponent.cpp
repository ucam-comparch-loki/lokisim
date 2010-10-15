/*
 * WrappedTileComponent.cpp
 *
 *  Created on: 17 Feb 2010
 *      Author: db434
 */

#include "WrappedTileComponent.h"
#include "TileComponentFactory.h"
#include "TileComponent.h"

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

Word WrappedTileComponent::getMemVal(int addr) const {
  return comp->getMemVal(addr);
}

int WrappedTileComponent::getRegVal(int reg) const {
  return comp->readReg(reg);
}

int WrappedTileComponent::getInstIndex() const {
  return comp->getInstIndex();
}

bool WrappedTileComponent::getPredReg() const {
  return comp->readPredReg();
}

void WrappedTileComponent::initialise() {
  fcOut.initialise();
}

void WrappedTileComponent::setup() {

// Initialise arrays
  dataIn          = new sc_in<AddressedWord>[NUM_CLUSTER_INPUTS];
  creditsOut      = new sc_out<AddressedWord>[NUM_CLUSTER_INPUTS];
  dataOut         = new sc_out<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  creditsIn       = new sc_in<AddressedWord>[NUM_CLUSTER_OUTPUTS];

  dataInSig       = new flag_signal<Word>[NUM_CLUSTER_INPUTS];
  dataOutSig      = new flag_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  dataOutSig2     = new flag_signal<AddressedWord>[NUM_CLUSTER_OUTPUTS];
  fcOutSig        = new sc_signal<bool>[NUM_CLUSTER_OUTPUTS];
  fcInSig         = new flag_signal<int>[NUM_CLUSTER_INPUTS];

// Connect everything up
  comp->clock(clock);
  fcOut.clock(clock);
  comp->idle(idle);

  for(uint i=0; i<NUM_CLUSTER_INPUTS; i++) {
    fcIn.dataIn[i](dataIn[i]);
    fcIn.credits[i](creditsOut[i]);

    fcIn.flowControl[i](fcInSig[i]); comp->flowControlOut[i](fcInSig[i]);
    fcIn.dataOut[i](dataInSig[i]); comp->in[i](dataInSig[i]);
  }

  for(uint i=0; i<NUM_CLUSTER_OUTPUTS; i++) {
    fcOut.dataOut[i](dataOutSig2[i]); dataOut[i](dataOutSig2[i]);
    fcOut.credits[i](creditsIn[i]);

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
