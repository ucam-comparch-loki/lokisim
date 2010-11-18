/*
 * RoundRobinArbiter.cpp
 *
 *  Created on: 16 Nov 2010
 *      Author: db434
 */

#include "RoundRobinArbiter.h"
#include <iostream>
#include <set>

vector<Path>& RoundRobinArbiter::arbitrate(vector<Path>& paths) {
  if(paths.empty()) return paths;

  vector<Path>* accepted = new vector<Path>();
  std::set<int> destinations;

  vector<Path>::iterator iter = paths.begin();

  // Find the position in the vector to start accepting requests.
  while(iter < paths.end() && iter->source() <= lastAccepted) {
    iter++;
  }

  // Return to the beginning of the vector and continue until we pass the
  // starting point.
  for(unsigned int j=0; j<paths.size(); j++) {
    // Loop the iterator back to the beginning if necessary.
    if(iter >= paths.end()) iter = paths.begin();

    ChannelIndex destination = iter->destination();//paths[j].destination();

    // Only accept a request if we have not already accepted a request sending
    // to the same destination.
    if(destinations.find(destination) == destinations.end()) {
      // Wormhole routing provides additional restrictions (once a packet
      // starts sending, it must complete), so check for those.
      if(wormholeAllows(*iter)) {
        accepted->push_back(*iter);
        destinations.insert(destination);
      }
    }

    iter++;
  }

  lastAccepted = accepted->back().source();
  return *accepted;
}

RoundRobinArbiter::RoundRobinArbiter(int numInputs, int numOutputs) :
    Arbiter(numInputs, numOutputs) {

}

RoundRobinArbiter::~RoundRobinArbiter() {

}
