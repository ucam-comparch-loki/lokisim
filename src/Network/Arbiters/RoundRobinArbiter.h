/*
 * RoundRobinArbiter.h
 *
 *  Created on: 28 Mar 2019
 *      Author: db434
 */

#ifndef SRC_NETWORK_ARBITERS_ROUNDROBINARBITER_H_
#define SRC_NETWORK_ARBITERS_ROUNDROBINARBITER_H_

#include "ArbiterBase.h"

class RoundRobinArbiter: public ArbiterBase {
public:
  RoundRobinArbiter() : ArbiterBase() {
    // Nothing
  }

  virtual PortIndex selectRequester(request_list_t& requests) {
    assert(!requests.empty());
    list<PortIndex> inputs = requests.getSorted();

    // Find the first requester with a higher index than the last accepted.
    // If there is no such requester, wrap around and choose the one with the
    // lowest index.
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
      if (*it > lastAccepted) {
        lastAccepted = *it;
        return lastAccepted;
      }
    }
    return inputs.front();
  }
};



#endif /* SRC_NETWORK_ARBITERS_ROUNDROBINARBITER_H_ */
