/*
 * Torus2D.h
 *
 * Like a mesh, but wraps around at all edges (i.e. the top-most row is
 * connected to the bottom-most row, and the left-most column is connected to
 * the right-most column).
 *
 * This is currently easier to model because there are no boundary conditions,
 * but it means there is no simple way of accessing off-chip resources.
 *
 *  Created on: 4 Nov 2010
 *      Author: db434
 */

#ifndef TORUS2D_H_
#define TORUS2D_H_

#include "../Network.h"

class Torus2D: public Network {

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Torus2D);
  Torus2D(sc_module_name name,
          ComponentID ID,
          ChannelID lowestID,   // Lowest channel ID accessible on this network
          ChannelID highestID,  // Highest channel ID accessible on this network
          int numComponents,   // Number of components connected to this network
          int numRows,
          int numColumns,
          int networkType);

  virtual ~Torus2D();

//==============================//
// Methods
//==============================//

private:

  // Compute which port to send the data out on, given the port it was received
  // on, and its ultimate destination.
  virtual ChannelIndex computeOutput(ChannelIndex source,
                                     ChannelID destination) const;

  virtual double distance(ChannelIndex inPort, ChannelIndex outPort) const;

//==============================//
// Methods
//==============================//

private:

  int numRows_, numColumns_;

};

#endif /* TORUS2D_H_ */
