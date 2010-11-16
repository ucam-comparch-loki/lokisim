/*
 * Crossbar.h
 *
 *  Created on: 3 Nov 2010
 *      Author: db434
 */

#ifndef CROSSBAR_H_
#define CROSSBAR_H_

#include "../Network.h"

class Crossbar: public Network {

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Crossbar);
  Crossbar(sc_module_name name,
           ComponentID ID,
           ChannelID lowestID,   // Lowest channel ID accessible on this network
           ChannelID highestID,  // Highest channel ID accessible on this network
           int numInputs,        // Number of inputs this network has
           int numOutputs,       // Number of outputs this network has
           Arbiter* arbiter);

  virtual ~Crossbar();

//==============================//
// Methods
//==============================//

private:

  // Compute which port to send the data out on, given the port it was received
  // on, and its ultimate destination.
  virtual ChannelIndex computeOutput(ChannelIndex source,
                                     ChannelID destination) const;

};

#endif /* CROSSBAR_H_ */
