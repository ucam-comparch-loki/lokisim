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

//==============================//
// Methods
//==============================//

public:

  virtual void route(input_port inputs[],
                     output_port outputs[],
                     int length,
                     std::vector<bool>& sent,
                     std::vector<bool>* blockedRequests);

private:

  void printMessage(int length, AddressedWord data, int from, int to);

//==============================//
// Constructors and destructors
//==============================//

public:

  Crossbar();
  virtual ~Crossbar();

};


#endif /* CROSSBAR_H_ */
