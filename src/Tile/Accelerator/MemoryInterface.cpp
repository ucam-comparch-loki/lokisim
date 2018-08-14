/*
 * MemoryInterface.cpp
 *
 *  Created on: 10 Aug 2018
 *      Author: db434
 */

#include "MemoryInterface.h"
#include "../../Utility/Assert.h"

MemoryInterface::MemoryInterface(sc_module_name name, ComponentID id) :
    LokiComponent(name, id),
    bankSelector(id) {

  outstandingRequests = 0;

  SC_METHOD(sendRequest);
  sensitive << requestArrived;
  dont_initialize();

}

void MemoryInterface::createNewRequest(position_t position, MemoryAddr address,
                                       MemoryOpcode op, uint32_t data=0) {
  assert(canAcceptRequest());

  request_t request;
  request.position = position;
  request.address = address;
  request.operation = op;
  request.data = data;

  requests.push(request);
  requestArrived.notify(sc_core::SC_ZERO_TIME);

  // It's only a new request if it isn't a payload.
  if (op != PAYLOAD && op != PAYLOAD_EOP)
    outstandingRequests++;
}

const response_t MemoryInterface::getResponse() {
  loki_assert(canGiveResponse());

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

void MemoryInterface::receiveResponse(const NetworkData& flit) {
  loki_assert(canAcceptResponse());

  uint32_t payload = flit.payload().toUInt();

  response_t response;
  response.data = payload;

  // Temporary: the destination PE for this data is held in a queue.
  response.position = inFlight.front();
  inFlight.pop();

  responses.push(response);

  responseArrived.notify(sc_core::SC_ZERO_TIME);
}

sc_event MemoryInterface::responseArrivedEvent() const {
  return responseArrived;
}

bool MemoryInterface::isIdle() const {
  return outstandingRequests == 0;
}

void MemoryInterface::replaceMemoryMapping(ChannelMapEntry::MemoryChannel mapping) {
  memoryMapping = mapping;
}

void MemoryInterface::sendRequest() {
  loki_assert(!requests.empty());

  request_t request = requests.front();
  requests.pop();

  // Temporary: store the PE position associated with this request so it can be
  // merged with the response from memory when it returns.
  inFlight.push(request.position);

  // TODO: ChannelID(id, 0)? Or encode position in the channel field. Or have
  // one channel per memory bank.
  ChannelID returnChannel(id, 0);

  // Instant memory access for now.
  parent()->magicMemoryAccess(request.operation, request.address, returnChannel,
                              request.data);

  if (!requests.empty())
    next_trigger(sc_core::SC_ZERO_TIME);
}

DMA* MemoryInterface::parent() const {
  return static_cast<DMA*>(this->get_parent_object());
}
