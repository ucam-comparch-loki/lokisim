/*
 * ArbiterBase.cpp
 *
 *  Created on: 2 Mar 2011
 *      Author: db434
 */

#include "ArbiterBase.h"
#include <assert.h>

void ArbiterBase::arbitrate(RequestList& requests, GrantList& grants) {
  // Skip everything if there is nothing to be done.
  if(requests.empty()) return;

  // Wipe the outputsUsed vector: nothing has been used yet.
  outputsUsed.assign(outputsUsed.size(), false);

  // Do the arbitration.
  arbitrate(requests, grants);
}

void ArbiterBase::wormholeGrant(Request& request, GrantList& grants) {
  // See if this request already has an output reserved.
  for(unsigned int i=0; i<reservations.size(); i++) {
    if(reservations[i] == request.first) {
      // Add a grant if we find a match.
      grant(request, i, grants);

      // Remove the reservation if this is the last flit of the packet.
      if(request.second) reservations[i] = NO_RESERVATION;

      return;
    }
  }

  // If we have made it this far, there were no reservations, so look for the
  // first available output.
  for(unsigned int i=0; i<reservations.size(); i++) {
    if(reservations[i] == NO_RESERVATION && !outputsUsed[i]) {
      grant(request, i, grants);

      // If this is the start of a packet, reserve the output for the rest of
      // the data.
      if(!request.second) reservations[i] = request.first;

      return;
    }
  }

  assert(false); // We should not have made it this far.

}

void ArbiterBase::grant(Request& request, int output, GrantList& grants) {
  assert(!outputsUsed[output]);
  grants.push_back(Grant(request.first, output));
  outputsUsed[output] = true;
}

ArbiterBase::ArbiterBase(int numInputs, int numOutputs, bool useWormhole) :
    reservations(numOutputs, NO_RESERVATION), // size = useWormhole?numOutputs:0 ?
    outputsUsed(numOutputs, false) {
  inputs = numInputs;
  outputs = numOutputs;
  wormhole = useWormhole;
}
