/*
 * MemoryInterface.cpp
 *
 *  Created on: 10 Aug 2018
 *      Author: db434
 */

#include "MemoryInterface.h"
#include "../../Utility/Assert.h"
#include "DMA.h"

MemoryInterface::MemoryInterface(sc_module_name name, ComponentID id) :
    LokiComponent(name, id),
    memoryMapping(-1),
    bankSelector(id) {

  outstandingRequests = 0;

  SC_METHOD(sendRequest);
  sensitive << requestArrived;
  dont_initialize();

}

void MemoryInterface::createNewRequest(position_t position, MemoryAddr address,
                                       MemoryOpcode op, uint32_t data) {
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

  LOKI_LOG << this->name() << ": received " << payload << " for " << response.position << endl;

  responseArrived.notify(sc_core::SC_ZERO_TIME);
}

const sc_event& MemoryInterface::responseArrivedEvent() const {
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

  if (request.operation != PAYLOAD && request.operation != PAYLOAD_EOP)
    LOKI_LOG << this->name() << ": accessing " << LOKI_HEX(request.address) << " for " << request.position << endl;

  // Temporary: store the PE position associated with this request so it can be
  // merged with the response from memory when it returns.
  inFlight.push(request.position);

  // TODO: ChannelID(id, 0)? Or encode position in the channel field. Or have
  // one channel per memory bank.
  ChannelID returnChannel(id, 0);

  // Instant memory access for now.
  parent().magicMemoryAccess(request.operation, request.address, returnChannel,
                             request.data);
//  if (request.operation == LOAD_AND_ADD)
//    cout << this->name() << " sending " << (int)request.data << " to " << LOKI_HEX(request.address) << endl;

  if (!requests.empty())
    next_trigger(sc_core::SC_ZERO_TIME);
}

DMA& MemoryInterface::parent() const {
  return *(static_cast<DMA*>(this->get_parent_object()));
}
/*
 * MemoryInterface.cpp
 *
 *  Created on: 10 Aug 2018
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "MemoryInterface.h"
#include "../../Utility/Assert.h"
#include "DMA.h"

using std::queue;

MemoryInterface::MemoryInterface(sc_module_name name, ComponentID id,
                                 uint numBanks) :
    LokiComponent(name),
    oRequest("oRequest", numBanks),
    iResponse("iResponse", numBanks),
    memoryMapping(-1),
    bankSelector(id),
    inFlight(numBanks, queue<position_t>()) {

  for (uint i=0; i<numBanks; i++) {
    // Request buffer has "infinite" capacity to allow us to do stuff like
    // write coalescing.
    NetworkFIFO<Word>* req = new NetworkFIFO<Word>(
        sc_gen_unique_name("requestBuf"), 100);
    requestBuffers.push_back(req);

    // Response buffer needs no capacity - it just forwards its data to the
    // staging area.
    NetworkFIFO<Word>* resp = new NetworkFIFO<Word>(
        sc_gen_unique_name("responseBuf"), 1);
    responseBuffers.push_back(resp);
  }

  oRequest(requestBuffers);
  iResponse(responseBuffers);

  outstandingRequests = 0;

  SC_METHOD(sendRequest);
  sensitive << requestArrived;
  dont_initialize();

  for (uint i=0; i<responseBuffers.size(); i++)
    SPAWN_METHOD(responseBuffers[i].canReadEvent(), MemoryInterface::processResponse, i, false);

}

void MemoryInterface::createNewRequest(position_t position, MemoryAddr address,
                                       MemoryOpcode op, uint payloadFlits,
                                       uint32_t data) {
  loki_assert(canAcceptRequest());

  request_t request;
  request.position = position;
  request.address = address;
  request.operation = op;
  request.hasPayload = (payloadFlits > 0);
  request.data = data;

  // TODO: put this directly in the output buffers?
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

  if (outstandingRequests == 0)
    becameIdle.notify(sc_core::SC_ZERO_TIME);

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
  // This is for magic memory access only. Normal networks write directly into
  // the buffers.
  uint buffer = flit.channelID().channel;
  responseBuffers[buffer].write(flit);
}

void MemoryInterface::processResponse(int buffer) {
  loki_assert(canAcceptResponse());
  loki_assert(responseBuffers[buffer].canRead());
  Flit<Word> flit = responseBuffers[buffer].read();

  uint32_t payload = flit.payload().toUInt();

  response_t response;
  response.data = payload;

  // The destination PE for this data is held in a queue.
  response.position = inFlight[buffer].front();
  inFlight[buffer].pop();

  responses.push(response);

  LOKI_LOG << this->name() << ": received " << payload << " for PE " << response.position << endl;

  responseArrived.notify(sc_core::SC_ZERO_TIME);
}

const sc_event& MemoryInterface::responseArrivedEvent() const {
  return responseArrived;
}

bool MemoryInterface::isIdle() const {
  return outstandingRequests == 0;
}

const sc_event& MemoryInterface::becameIdleEvent() const {
  return becameIdle;
}

void MemoryInterface::replaceMemoryMapping(ChannelMapEntry::MemoryChannel mapping) {
  memoryMapping = mapping;
}

void MemoryInterface::sendRequest() {
  loki_assert(!requests.empty());

  request_t request = requests.front();
  requests.pop();

  if (request.operation != PAYLOAD && request.operation != PAYLOAD_EOP)
    LOKI_LOG << this->name() << ": accessing " << LOKI_HEX(request.address) << " for PE " << request.position << endl;

  ChannelID memoryBank = bankSelector.getMapping(request.operation,
      request.address, memoryMapping);
  ComponentIndex bankID = memoryBank.component.position;
  memoryBank.component.position += CORES_PER_TILE;  // Hack

  // Store the PE position associated with this request so it can be
  // merged with the response from memory when it returns.
  inFlight[bankID].push(request.position);


  if (MAGIC_MEMORY) {
    ChannelID returnChannel(id(), bankID);
    parent().magicMemoryAccess(request.operation, request.address,
                               returnChannel, request.data);
  }
  else {
    loki_assert(requestBuffers[bankID].canWrite());

    // Adjust the memory mapping so data is returned to a specific buffer.
    ChannelMapEntry::MemoryChannel mapping = memoryMapping;
    mapping.returnChannel = bankID;

    // Head flit.
    Flit<Word> head(request.address, memoryBank, mapping,
                    request.operation, !request.hasPayload);
    requestBuffers[bankID].write(head);

    // Payload flit.
    if (request.hasPayload) {
      loki_assert(requestBuffers[bankID].canWrite());

      Flit<Word> payload(request.data, memoryBank, mapping, PAYLOAD_EOP, true);
      requestBuffers[bankID].write(payload);
    }

  }
//  if (request.operation == LOAD_AND_ADD)
//    cout << this->name() << " sending " << (int)request.data << " to " << LOKI_HEX(request.address) << endl;

  if (!requests.empty())
    next_trigger(sc_core::SC_ZERO_TIME);
}

DMA& MemoryInterface::parent() const {
  return *(static_cast<DMA*>(this->get_parent_object()));
}

const ComponentID& MemoryInterface::id() const {
  return parent().id;
}
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
    oRequest("oRequest"),
    iResponse("iResponse"),
    memory(memory),
    requestBuffer("requestBuf", 100),   // "Infinite" capacity
    responseBuffer("responseBuf", 8) {

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

  LOKI_LOG(3) << this->name() << ": received " << payload << " for PE "
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

  request_t request = requests.front();
  requests.pop();

  if (request.operation != PAYLOAD && request.operation != PAYLOAD_EOP)
    LOKI_LOG(3) << this->name() << ": accessing " << LOKI_HEX(request.address) << " for PE " << request.position << endl;

  // The network address of the memory to access.
  ChannelID memoryBank(id().tile, memory + CORES_PER_TILE, parent().memoryMapping.channel);

  // TODO: only do this if we expect a response.
  inFlight.push(request);

  if (MAGIC_MEMORY) {
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

  if (!requests.empty())
    next_trigger(sc_core::SC_ZERO_TIME);
}

DMA& MemoryInterface::parent() const {
  return *(static_cast<DMA*>(this->get_parent_object()));
}

const ComponentID& MemoryInterface::id() const {
  return parent().id;
}
