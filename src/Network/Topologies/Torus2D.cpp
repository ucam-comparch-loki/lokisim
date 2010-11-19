/*
 * Torus2D.cpp
 *
 *  Created on: 4 Nov 2010
 *      Author: db434
 */

#include "Torus2D.h"
#include "../Router.h"
#include "../Arbiters/NullArbiter.h"

// For the Torus, we assume that each node has exactly 4 inputs and 4 outputs,
// corresponding to north, east, south and west. The nodes may also have a
// fifth input/output (local), but that will not be connected to this network.
ChannelIndex Torus2D::computeOutput(ChannelIndex source,
                                    ChannelID destination) const {
  // The torus itself is wired up, with the routers controlling where data
  // goes. The network therefore doesn't need to do anything.
  return 0;
}

/* Currently assumes that the torus will only be used for the global network,
 * meaning each hop will be the size of a tile: 1mm. */
double Torus2D::distance(ChannelIndex inPort, ChannelIndex outPort) const {
  return 1.0;
}

Torus2D::Torus2D(sc_module_name name,
                 ComponentID ID,
                 ChannelID lowestID,
                 ChannelID highestID,
                 int numComponents,
                 int numRows,
                 int numColumns,
                 int networkType) :
    Network(name, ID, lowestID, highestID, numComponents*4, numComponents*4, networkType) {

  // TODO: make routers in here, and wire everything up.

  numRows_ = numRows;
  numColumns_ = numColumns;

  // The torus doesn't need any arbitration because it has routers which do
  // the hard work.
  arbiter_ = new NullArbiter(numInputs, numOutputs);

}

Torus2D::~Torus2D() {

}
