/*
 * ArbiterBase.h
 *
 *  Created on: 28 Mar 2019
 *      Author: db434
 */

#ifndef SRC_NETWORK_ARBITERS_ARBITERBASE_H_
#define SRC_NETWORK_ARBITERS_ARBITERBASE_H_

#include "../../Types.h"
#include <list>

using sc_core::sc_event;
using std::list;

// Datatype used to accumulate requests before passing them to an arbiter.
class request_list_t;

class ArbiterBase {
public:
  ArbiterBase() {
    lastAccepted = -1;  // Start at -1, so any input is acceptable first time.
    held = false;
  }
  virtual ~ArbiterBase() {}

  // Choose the next input to use data from. This input may not necessarily be
  // in the given list of requesters (e.g. if we are in wormhole mode and forced
  // to choose the same input as last time).
  PortIndex arbitrate(request_list_t& requests) {
    // Repeat the last selected input when in wormhole routing mode.
    // This ignores whether there is a fresh request from that input, so this
    // must be checked elsewhere.
    if (held)
      return lastAccepted;

    PortIndex selected = selectRequester(requests);

    lastAccepted = selected;
    return selected;
  }

  void hold()    {held = true;}
  void release() {held = false;}

protected:
  // The method to be overridden by subclasses. Select an input from the given
  // list of requesters.
  virtual PortIndex selectRequester(request_list_t& requests) = 0;

  // The input port selected last time.
  PortIndex lastAccepted;

  // When true, only allow lastAccepted to be selected. Used for wormhole
  // routing.
  bool held;
};

class request_list_t {
public:
  void add(PortIndex input) {
    requests.push_back(input);
    newRequest.notify(sc_core::SC_ZERO_TIME);
  }

  void remove(PortIndex input) {
    requests.remove(input);
  }

  const list<PortIndex>& getSorted() {
    requests.sort();
    return requests;
  }

  bool empty() const {
    return requests.empty();
  }

  bool contains(PortIndex input) const {
    for (auto it=requests.begin(); it != requests.end(); ++it)
      if (*it == input)
        return true;

    return false;
  }

  const sc_event& newRequestEvent() const {
    return newRequest;
  }

private:
  // Super simple implementation for now because I expect very small lists.
  list<PortIndex> requests;
  sc_event newRequest;
};



#endif /* SRC_NETWORK_ARBITERS_ARBITERBASE_H_ */
