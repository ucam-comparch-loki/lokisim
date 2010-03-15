/*
 * RoutingSchemeFactory.h
 *
 * Factory for generating various types of routers.
 *
 *  Created on: 15 Mar 2010
 *      Author: db434
 */

#ifndef ROUTINGSCHEMEFACTORY_H_
#define ROUTINGSCHEMEFACTORY_H_

#include "RoutingScheme.h"

class RoutingSchemeFactory {

public:
  static RoutingScheme* makeRoutingScheme();

};

#endif /* ROUTINGSCHEMEFACTORY_H_ */
