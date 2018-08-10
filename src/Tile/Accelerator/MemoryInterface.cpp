/*
 * MemoryInterface.cpp
 *
 *  Created on: 10 Aug 2018
 *      Author: db434
 */

#include "MemoryInterface.h"

using sc_core::sc_event;

MemoryInterface::MemoryInterface() {
  outstandingRequests = 0;

}

void MemoryInterface::sendRequest(const request_t request) {
  assert(canAcceptRequest());

  // TODO Update bank and return address if necessary.
  requests.push(request);
  requestArrived.notify(sc_core::SC_ZERO_TIME);

  // It's only a new request if it isn't a payload.
  if (request.getMemoryMetadata().opcode != PAYLOAD &&
      request.getMemoryMetadata().opcode != PAYLOAD_EOP)
    outstandingRequests++;
}

const response_t MemoryInterface::getResponse() {
  assert(canGiveResponse());
  const response_t response = responses.front();
  responses.pop();
  outstandingRequests--;

  return response;
}

bool MemoryInterface::canAcceptRequest() const {
  // Infinite capacity for now.
  return true;
}

bool MemoryInterface::canAcceptResponse() const {
  // Infinite capacity for now.
  return true;
}

bool MemoryInterface::canGiveResponse() const {
  return !responses.empty();
}

sc_event MemoryInterface::responseArrivedEvent() const {
  return responseArrived;
}

bool MemoryInterface::isIdle() const {
  return outstandingRequests == 0;
}

void MemoryInterface::requestsToMemory() {
  // TODO
}

void MemoryInterface::receiveResponse(/*TODO params*/) {
  // TODO
  // responses.push(something);

  responseArrived.notify(sc_core::SC_ZERO_TIME);
}
