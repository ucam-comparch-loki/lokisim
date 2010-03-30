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
  virtual void route(sc_in<AddressedWord> *inputs, sc_out<Word> *outputs,
                     int length, std::vector<bool>& sent) = 0;

};

#endif /* ROUTINGSCHEME_H_ */
