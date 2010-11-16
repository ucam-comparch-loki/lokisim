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
  unsigned int start=0;

  // Find the position in the vector to start accepting requests.
  while(start<paths.size() && paths[start].source() <= lastAccepted) {
    start++;
  }

  // Continue until we hit the end of the vector, accepting everything possible.
  for(unsigned int j=start; j<paths.size(); j++) {
    int destination = paths[j].destination();

    // Only accept a request if we have not already accepted a request sending
    // to the same destination.
    if(destinations.find(destination) == destinations.end()) {
      accepted->push_back(paths[j]);
      destinations.insert(destination);
    }
  }

  // Return to the beginning of the vector and continue until we pass the
  // starting point.
  for(unsigned int j=0; j<start; j++) {
    int destination = paths[j].destination();

    // Only accept a request if we have not already accepted a request sending
    // to the same destination.
    if(destinations.find(destination) == destinations.end()) {
      accepted->push_back(paths[j]);
      destinations.insert(destination);
    }
  }

  lastAccepted = accepted->back().source();
  return *accepted;
}

RoundRobinArbiter::RoundRobinArbiter(int numInputs, int numOutputs) :
    Arbiter(numInputs, numOutputs) {

}

RoundRobinArbiter::~RoundRobinArbiter() {

}
