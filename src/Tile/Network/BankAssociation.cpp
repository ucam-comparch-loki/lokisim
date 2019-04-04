/*
 * BankAssociation.cpp
 *
 *  Created on: 4 Apr 2019
 *      Author: db434
 */

#include "BankAssociation.h"
#include "../../Utility/Assert.h"

BankAssociation::BankAssociation(sc_module_name name, uint numBanks) :
    LokiComponent(name),
    numBanks(numBanks) {

  // Pretend we have finished a request (allows some more-aggressive
  // assertions).
  numResponses = numBanks;
  hit = false;
  target = -1;

}

// The bank responsible for handling this request if all banks miss.
MemoryIndex BankAssociation::targetBank() const {
  return target;
}

// Notify of a cache hit.
void BankAssociation::cacheHit() {
  loki_assert_with_message(!hit, "Two cache hits for same request", 0);

  hit = true;
  addResponse();
}

// Notify of a cache miss.
void BankAssociation::cacheMiss() {
  addResponse();
}

// Check whether all banks have responded with their hit/miss status.
// If the target bank has a cache miss, it is only allowed to begin a request
// once all banks have responded.
bool BankAssociation::allResponsesReceived() const {
  return (numResponses == numBanks);
}

const sc_event& BankAssociation::allResponsesReceivedEvent() const {
  return finalResponseReceived;
}

// Returns whether one of the banks had a cache hit.
bool BankAssociation::associativeHit() const {
  return hit;
}

// Clear any state and begin a new request.
void BankAssociation::newRequest(MemoryIndex targetBank) {
  loki_assert(allResponsesReceived());
  loki_assert(targetBank < numBanks);

  numResponses = 0;
  hit = false;
  target = targetBank;
}

// Updates performed when any response is received (hit or miss).
void BankAssociation::addResponse() {
  loki_assert(!allResponsesReceived());

  numResponses++;

  if (allResponsesReceived())
    finalResponseReceived.notify(sc_core::SC_ZERO_TIME);
}
