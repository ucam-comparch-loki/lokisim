/*
 * L2LToBankRequests.cpp
 *
 *  Created on: 4 Apr 2019
 *      Author: db434
 */

#include "../../Utility/Assert.h"
#include "L2LToBankRequests.h"

L2LToBankRequests::L2LToBankRequests(sc_module_name name, uint numBanks) :
    LokiComponent(name),
    numBanks(numBanks) {

  // Pretend we have finished a request (allows some more-aggressive
  // assertions).
  numResponses = numBanks;
  hit = false;
  consumed = true;
  target = -1;

}

// Read the flit currently on the channel.
const NetworkRequest& L2LToBankRequests::peek() const {
  return flit;
}

const NetworkRequest& L2LToBankRequests::read() {
  loki_assert_with_message(!consumed, "Two reads of same flit", 0);

  consumed = true;
  if (canWrite())
    finishedWithFlit.notify(sc_core::SC_ZERO_TIME);

  return flit;
}

// The bank responsible for handling this request if all banks miss.
MemoryIndex L2LToBankRequests::targetBank() const {
  return target;
}

// Notify of a cache hit.
void L2LToBankRequests::cacheHit() {
  loki_assert_with_message(!hit, "Two cache hits for same request", 0);

  hit = true;
  addResponse();
}

// Notify of a cache miss.
void L2LToBankRequests::cacheMiss() {
  addResponse();
}

// Check whether all banks have responded with their hit/miss status.
// If the target bank has a cache miss, it is only allowed to begin a request
// once all banks have responded.
bool L2LToBankRequests::allResponsesReceived() const {
  return (numResponses == numBanks);
}

const sc_event& L2LToBankRequests::allResponsesReceivedEvent() const {
  return finalResponseReceived;
}

// Returns whether one of the banks had a cache hit.
bool L2LToBankRequests::associativeHit() const {
  return hit;
}

// Event triggered when the first flit of a new request arrives.
const sc_event& L2LToBankRequests::newRequestArrived() const {
  return newRequestEvent;
}

// Event triggered whenever any flit arrives.
const sc_event& L2LToBankRequests::newFlitArrived() const {
  return newFlitEvent;
}

// Clear any state and begin a new request.
void L2LToBankRequests::newRequest(NetworkRequest request, MemoryIndex targetBank) {
  loki_assert(allResponsesReceived());
  loki_assert(targetBank < numBanks);

  numResponses = 0;
  hit = false;
  consumed = false;
  flit = request;
  target = targetBank;

  newRequestEvent.notify(sc_core::SC_ZERO_TIME);
  newFlitEvent.notify(sc_core::SC_ZERO_TIME);
}

// Supply a new flit for an existing request.
void L2LToBankRequests::newPayload(NetworkRequest request) {
  loki_assert(allResponsesReceived());

  consumed = false;
  flit = request;

  newFlitEvent.notify(sc_core::SC_ZERO_TIME);
}

bool L2LToBankRequests::canWrite() const {
  return consumed && allResponsesReceived();
}

const sc_event& L2LToBankRequests::canWriteEvent() const {
  return finishedWithFlit;
}

// Updates performed when any response is received (hit or miss).
void L2LToBankRequests::addResponse() {
  loki_assert(!allResponsesReceived());

  numResponses++;

  if (allResponsesReceived())
    finalResponseReceived.notify(sc_core::SC_ZERO_TIME);

  if (canWrite())
    finishedWithFlit.notify(sc_core::SC_ZERO_TIME);
}
