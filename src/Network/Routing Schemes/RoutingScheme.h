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

public:
/* Methods */
  virtual void route(sc_in<AddressedWord> *inputs, sc_out<Word> *outputs,
                     int length) = 0;

};

#endif /* ROUTINGSCHEME_H_ */
