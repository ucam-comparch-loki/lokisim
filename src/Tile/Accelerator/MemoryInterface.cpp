/*
 * MemoryInterface.cpp
 *
 *  Created on: 10 Aug 2018
 *      Author: db434
 */

#include "MemoryInterface.h"

#include "../../Utility/Assert.h"
#include "DMA.h"

using std::queue;

MemoryInterface::MemoryInterface(sc_module_name name, ComponentIndex memory) :
    LokiComponent(name),
    BlockingInterface(),
    clock("clock"),
    oRequest("oRequest"),
    iResponse("iResponse"),
    memory(memory),
    requestBuffer("requestBuf", 100, 100), // "Infinite" capacity
    responseBuffer("responseBuf", 8, 100) { // TODO: bandwidth

  requestBuffer.clock(clock);
  responseBuffer.clock(clock);

  oRequest(requestBuffer);
  iResponse(responseBuffer);

  SC_METHOD(sendRequest);
  sensitive << requestArrived;
  dont_initialize();

  SC_METHOD(processResponse);
  sensitive << responseBuffer.canReadEvent();
  dont_initialize();

}

void MemoryInterface::createNewRequest(tick_t tick, position_t position,
                                       MemoryAddr address, MemoryOpcode op,
                                       uint payloadFlits, uint32_t data) {
  loki_assert(canAcceptRequest());

  request_t request;
  request.tick = tick;
  request.position = position;
  request.address = address;
  request.operation = op;
  request.hasPayload = (payloadFlits > 0);
  request.data = data;

  // TODO: put this directly in the output buffer?
  requests.push(request);
  requestArrived.notify(sc_core::SC_ZERO_TIME);
}

const response_t MemoryInterface::getResponse() {
  loki_assert(canGiveResponse());

  const response_t response = responses.front();
  responses.pop();

  if (isIdle())
    becameIdle.notify(sc_core::SC_ZERO_TIME);

  return response;
}

tick_t MemoryInterface::currentTick() const {
  loki_assert(canGiveResponse());
  return responses.front().tick;
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
  // This is for magic memory access only. Normal networks write directly into
  // the buffers.
  responseBuffer.write(flit);
}

void MemoryInterface::processResponse() {
  loki_assert(canAcceptResponse());
  loki_assert(responseBuffer.canRead());

  Flit<Word> flit = responseBuffer.read();
  loki_assert_with_message(!inFlight.empty(), "Flit = %s", flit.getString().c_str());

  int32_t payload = flit.payload().toInt();

  // The destination PE (and other info) for this data is held in a queue.
  request_t request = inFlight.front();
  inFlight.pop();

  response_t response;
  response.tick = request.tick;
  response.position = request.position;
  response.data = payload;
  responses.push(response);

  LOKI_LOG(2) << this->name() << ": received " << payload << " for PE "
      << response.position << ", tick " << request.tick << endl;

  responseArrived.notify(sc_core::SC_ZERO_TIME);
}

const sc_event& MemoryInterface::responseArrivedEvent() const {
  return responseArrived;
}

bool MemoryInterface::isIdle() const {
  return requests.empty() && responses.empty() && inFlight.empty();
}

const sc_event& MemoryInterface::becameIdleEvent() const {
  return becameIdle;
}

void MemoryInterface::reportStalls(ostream& os) {
  if (!requests.empty())
    os << this->name() << " has unprocessed requests" << endl;
  if (!inFlight.empty()) {
    os << this->name() << " has requests in flight" << endl;
    request_t head = inFlight.front();
    os << "  Head of queue: " << memoryOpName(head.operation) << " for " << LOKI_HEX(head.address) << endl;
  }
  if (!responses.empty())
    os << this->name() << " has responses ready" << endl;
}

void MemoryInterface::sendRequest() {
  loki_assert(!requests.empty());

  if (!requestBuffer.canWrite()) {
    next_trigger(requestBuffer.canWriteEvent());
    return;
  }

  request_t request = requests.front();
  // Pop from queue right at the end.

  // The network address of the memory to access.
  ChannelID memoryBank(id().tile, memory + CORES_PER_TILE, parent().memoryMapping.channel);

  if (MAGIC_MEMORY) {
    // Response is immediate and bypasses flow control, so add extra check.
    if (!responseBuffer.canWrite()) {
      next_trigger(responseBuffer.canWriteEvent());
      return;
    }

    ChannelID returnChannel(id(), memory);
    parent().magicMemoryAccess(request.operation, request.address,
                               returnChannel, request.data);
  }
  else {
    loki_assert(requestBuffer.canWrite());

    // Adjust the memory mapping so data is returned to a specific buffer.
    ChannelMapEntry::MemoryChannel mapping = parent().memoryMapping;
    mapping.returnChannel = memory;

    // Head flit.
    Flit<Word> head(request.address, memoryBank, mapping,
                    request.operation, !request.hasPayload);
    requestBuffer.write(head);

    // Payload flit.
    if (request.hasPayload) {
      loki_assert(requestBuffer.canWrite());

      Flit<Word> payload(request.data, memoryBank, mapping, PAYLOAD_EOP, true);
      requestBuffer.write(payload);
    }

  }
//  if (request.operation == LOAD_AND_ADD)
//    cout << this->name() << " sending " << (int)request.data << " to " << LOKI_HEX(request.address) << endl;

  if (request.operation != PAYLOAD && request.operation != PAYLOAD_EOP)
    LOKI_LOG(2) << this->name() << ": accessing " << LOKI_HEX(request.address)
        << " for PE " << request.position << ", tick " << request.tick << endl;

  // TODO: only do this if we expect a response.
  inFlight.push(request);

  requests.pop();

  if (!requests.empty())
    next_trigger(sc_core::SC_ZERO_TIME);
}

DMA& MemoryInterface::parent() const {
  return *(static_cast<DMA*>(this->get_parent_object()));
}

const ComponentID& MemoryInterface::id() const {
  return parent().id;
}
