/*
 * RoundRobinArbiter.cpp
 *
 *  Created on: 2 Mar 2011
 *      Author: db434
 */

#include "RoundRobinArbiter.h"

void RoundRobinArbiter2::arbitrateLogic(RequestList& requests, GrantList& grants) {
  RequestList::iterator iter = requests.begin();

  // Find the position in the list to start accepting requests.
  while(iter < requests.end() && iter->first <= lastAccepted) {
    iter++;
  }

  // Return to the beginning of the vector and continue until we pass the
  // starting point.
  for(unsigned int i=0; i<requests.size(); i++) {
    // Loop the iterator back to the beginning if necessary.
    if(iter >= requests.end()) iter = requests.begin();

    if(wormhole) {
      wormholeGrant(*iter, grants);
    }
    else {
      // We are definitely allowed to send.
      grant(*iter, grants.size(), grants);
    }

    if(grants.size() >= outputs) break;

    iter++;
  }

  if(!grants.empty()) lastAccepted = grants.back().first;
  return;
}

RoundRobinArbiter2::RoundRobinArbiter2(int numInputs, int numOutputs, bool wormhole) :
    ArbiterBase(numInputs, numOutputs, wormhole) {

}
