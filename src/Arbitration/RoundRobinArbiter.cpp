/*
 * RoundRobinArbiter.cpp
 *
 *  Created on: 2 Mar 2011
 *      Author: db434
 */

#include "RoundRobinArbiter.h"
#include <iostream>

int RoundRobinArbiter::getGrant() {

  for(int i=0; i<numInputs(); i++) {
    // Increment pointer, wrapping around if necessary.
    lastAccepted++;
    if (lastAccepted >= numInputs())
      lastAccepted = 0;

    // Accept a request if there is a request, and it hasn't already been
    // granted.
    if (requests->at(lastAccepted) && !grants->at(lastAccepted))
      return lastAccepted;
  }

  // If we escape the loop, there are no requests we can grant.
  return NO_GRANT;
}

RoundRobinArbiter::RoundRobinArbiter(const RequestList* requestVec,
                                     const GrantList* grantVec) :
    ArbiterBase(requestVec, grantVec) {

  // Start at -1, so when incremented for the first time, we start looking at 0.
  lastAccepted = -1;

}
