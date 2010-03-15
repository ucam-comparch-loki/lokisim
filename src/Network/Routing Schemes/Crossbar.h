/*
 * Crossbar.h
 *
 * Simple implementation of a crossbar router.
 *
 *  Created on: 15 Mar 2010
 *      Author: db434
 */

#ifndef CROSSBAR_H_
#define CROSSBAR_H_

#include "RoutingScheme.h"

class Crossbar : public RoutingScheme {

public:
/* Methods */
  virtual void route(sc_in<AddressedWord> *inputs, sc_out<Word> *outputs,
                     int length);

/* Constructors and destructors */
  Crossbar();
  virtual ~Crossbar();

};


#endif /* CROSSBAR_H_ */
