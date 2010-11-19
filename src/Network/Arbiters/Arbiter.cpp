/*
 * Arbiter.cpp
 *
 *  Created on: 16 Nov 2010
 *      Author: db434
 */

#include "Arbiter.h"
#include "NullArbiter.h"
#include "RoundRobinArbiter.h"
#include "../../Utility/Parameters.h"

Arbiter* Arbiter::localDataArbiter(int numInputs, int numOutputs) {
  return new RoundRobinArbiter(numInputs, numOutputs);
}

Arbiter* Arbiter::localCreditArbiter(int numInputs, int numOutputs) {
  return new RoundRobinArbiter(numInputs, numOutputs);
}

Arbiter* Arbiter::globalDataArbiter(int numInputs, int numOutputs) {
  return new RoundRobinArbiter(numInputs, numOutputs);
}

Arbiter* Arbiter::globalCreditArbiter(int numInputs, int numOutputs) {
  return new RoundRobinArbiter(numInputs, numOutputs);
}

bool Arbiter::wormholeAllows(Path& path) {
  if(WORMHOLE_ROUTING) {
    vector<Path>::iterator iter;

    // Check open connections for communications with the same destination.
    for(iter = openConnections.begin(); iter<openConnections.end(); iter++) {
      if(iter->destination() == path.destination()) {
        if(iter->source() == path.source()) {
          // Remove the connection if the packet has now completed.
          if(path.data().endOfPacket()) openConnections.erase(iter);
          return true;
        }
        else {
          std::cout << "Can't send " << path.data() << " because of " << iter->data() << std::endl;
          return false;
        }
      }
    }

    // If we have got this far, there are no conflicting connections open.

    // Start a connection if there is more of the packet to come.
    if(!path.data().endOfPacket()) openConnections.push_back(path);

    return true;
  }
  // If not using wormhole routing, apply no additional restrictions.
  else return true;
}

Arbiter::Arbiter(int numInputs, int numOutputs) {
  inputs = numInputs;
  outputs = numOutputs;
}

Arbiter::~Arbiter() {

}
