/*
 * Torus2D.cpp
 *
 *  Created on: 4 Nov 2010
 *      Author: db434
 */

#include "Torus2D.h"
#include "../Router.h"

// For the Torus, we assume that each node has exactly 4 inputs and 4 outputs,
// corresponding to north, east, south and west. The nodes may also have a
// fifth input/output (local), but that will not be connected to this network.
ChannelIndex Torus2D::computeOutput(ChannelIndex source,
                                    ChannelID destination) const {
  // The torus itself is wired up, with the routers controlling where data
  // goes. The network therefore doesn't need to do anything.
  return 0;
}

Torus2D::Torus2D(sc_module_name name,
                 ComponentID ID,
                 ChannelID lowestID,
                 ChannelID highestID,
                 int numComponents,
                 int numRows,
                 int numColumns) :
    Network(name, ID, lowestID, highestID, numComponents*4, numComponents*4) {

  // TODO: make routers in here, and wire everything up.

  numRows_ = numRows;
  numColumns_ = numColumns;

}

Torus2D::~Torus2D() {

}
