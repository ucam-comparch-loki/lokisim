/*
 * Crossbar.cpp
 *
 *  Created on: 3 Nov 2010
 *      Author: db434
 */

#include "Crossbar.h"
#include "../../Datatype/AddressedWord.h"
#include "../Arbiters/RoundRobinArbiter.h"

ChannelIndex Crossbar::computeOutput(ChannelIndex source,
                                     ChannelID destination) const {
  if(destination<startID || destination>=endID) return offNetworkOutput;
  else return (destination-startID)/idsPerChannel;
}

Crossbar::Crossbar(sc_module_name name,
                   ComponentID ID,
                   ChannelID lowestID,
                   ChannelID highestID,
                   int numInputs,
                   int numOutputs,
                   Arbiter* arbiter) :
    Network(name, ID, lowestID, highestID, numInputs, numOutputs) {

  arbiter_ = arbiter;

}

Crossbar::~Crossbar() {

}
