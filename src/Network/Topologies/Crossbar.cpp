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
  // Not sure if this is right, but it seems to work for now.
  // Would expect to have ">= endID" instead of only "> endID".
  if(destination<startID || destination>endID) return offNetworkOutput;
  else return (destination-startID)/idsPerChannel;
}

/* Computes the distance a signal must travel through the crossbar.
 * Assumes that the ports are split in half: half above the crossbar and half
 * below (except for the connections to the next level of the hierarchy, which
 * are at one end). Also assumes a constant height for the crossbar. */
double Crossbar::distance(ChannelIndex inPort, ChannelIndex outPort) const {
  static double crossbarHeight = 0.2;
  static double crossbarWidth = 1.0;  // We expect tiles to be 1mm^2
  static double inPortSpacing = crossbarWidth/(numInputs/2);
  static double outPortSpacing = crossbarWidth/(numOutputs/2);

  // Not sure if this is correct.
  double hDist = (inPort%(numInputs/2)) * inPortSpacing -
                 (outPort%(numOutputs/2)) * outPortSpacing;
  if(hDist < 0) hDist = -hDist;

  // See if both ports are on the top/bottom, or if they are on different halves.
  double vDist = (inPort/(numInputs/2)) == (outPort/(numOutputs/2)) ? 0
                                                                    : crossbarHeight;
  return hDist + vDist;
}

Crossbar::Crossbar(sc_module_name name,
                   ComponentID ID,
                   ChannelID lowestID,
                   ChannelID highestID,
                   int numInputs,
                   int numOutputs,
                   int networkType,
                   Arbiter* arbiter) :
    Network(name, ID, lowestID, highestID, numInputs, numOutputs, networkType) {

  arbiter_ = arbiter;

}

Crossbar::~Crossbar() {

}
