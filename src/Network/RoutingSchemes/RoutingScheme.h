/*
 * RoutingScheme.h
 *
 * Base class for all routing schemes.
 *
 *  Created on: 15 Mar 2010
 *      Author: db434
 */

#ifndef ROUTINGSCHEME_H_
#define ROUTINGSCHEME_H_

#include "../../Component.h"
#include "../../Datatype/AddressedWord.h"

typedef sc_in<AddressedWord>  input_port;
typedef sc_out<AddressedWord> output_port;

class RoutingScheme {

//==============================//
// Methods
//==============================//

public:

  // Route information from the inputs to the outputs.
  //   inputs  = array of input ports
  //   outputs = array of output ports
  //   length  = length of input array
  //   sent    = vector telling whether it is allowed to write to each output
  //   blockedRequests = vector to return whether some of the messages blocked
  //   instrumentation = flag saying whether data should be recorded
  virtual void route(input_port inputs[],
                     output_port outputs[],
                     int length,
                     std::vector<bool>& sent,
                     bool instrumentation) = 0;

protected:

  // Calculates the distance data will travel to communicate from one component
  // to another. Units are the width of a cluster.
  virtual double distance(int startID, int endID) = 0;

};

#endif /* ROUTINGSCHEME_H_ */
