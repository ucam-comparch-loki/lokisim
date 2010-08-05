/*
 * RoutingSchemeFactory.cpp
 *
 *  Created on: 15 Mar 2010
 *      Author: db434
 */

#include "RoutingSchemeFactory.h"
#include "Crossbar.h"

RoutingScheme* RoutingSchemeFactory::makeRoutingScheme() {
  return new Crossbar();
}
