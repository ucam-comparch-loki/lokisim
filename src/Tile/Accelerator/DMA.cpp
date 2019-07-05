/*
 * DMA.cpp
 *
 *  Created on: 21 Aug 2018
 *      Author: db434
 */

#include "DMA.h"
#include "Accelerator.h"

DMA::DMA(sc_module_name name, ComponentID id, size_t queueLength) :
    LokiComponent(name, id),
    iCommand("iCommand"),
    iTick("iTick"),
    commandQueue(queueLength),
    memoryInterface("ifc", id) {

  SC_METHOD(newCommandArrived);
  sensitive << iCommand;
  dont_initialize();

}

void DMA::enqueueCommand(const dma_command_t command) {
  commandQueue.enqueue(command);
}

bool DMA::canAcceptCommand() const {
  return !commandQueue.full();
}

void DMA::replaceMemoryMapping(EncodedCMTEntry mapEncoded) {
  ChannelMapEntry::MemoryChannel decoded =
      ChannelMapEntry::memoryView(mapEncoded);

  // Memory decides which component to return data to using the memory
  // channel accessed. Set the channel appropriately.
  // Components are currently ordered: cores, memories, accelerators, but
  // memories cannot access memories, so remove them from consideration.
  decoded.channel = id.position - MEMS_PER_TILE;

  memoryInterface.replaceMemoryMapping(decoded);
}

// Magic connection from memory.
void DMA::deliverDataInternal(const NetworkData& flit) {
  memoryInterface.receiveResponse(flit);
}

// Remove and return the next command from the control unit.
const dma_command_t DMA::dequeueCommand() {
  return commandQueue.dequeue();
}

// Generate a new memory request and send it to the appropriate memory bank.
void DMA::createNewRequest(position_t position, MemoryAddr address,
                           MemoryOpcode op, int data) {
  memoryInterface.createNewRequest(position, address, op, data);
}

// Send a request to memory. If a response is expected, it will trigger the
// memoryInterface.responseArrivedEvent() event.
void DMA::memoryAccess(position_t position, MemoryAddr address, MemoryOpcode op,
                       int data) {
  switch (op) {
    case LOAD_W:
      loadData(position, address);
      break;
    case STORE_W:
      storeData(position, address, data);
      break;
    case LOAD_AND_ADD:
      loadAndAdd(position, address, data);
      break;
    default:
      throw InvalidOptionException("Convolution memory operation", op);
      break;
  }
}

// Instantly send a request to memory. This is not a substitute for
// memoryAccess above: this method is only to be called once all details about
// the access have been confirmed.
void DMA::magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address,
                            ChannelID returnChannel, Word data) {
  parent()->magicMemoryAccess(opcode, address, returnChannel, data);
}

Accelerator* DMA::parent() const {
  return static_cast<Accelerator*>(this->get_parent_object());
}

void DMA::newCommandArrived() {
  loki_assert(canAcceptCommand());
  enqueueCommand(iCommand.read());
}


/*
 * DMA.cpp
 *
 *  Created on: 21 Aug 2018
 *      Author: db434
 */

#include "DMA.h"
#include "Accelerator.h"

DMA::DMA(sc_module_name name, ComponentID id, uint numBanks, size_t queueLength) :
    LokiComponent(name),
    iCommand("iCommand"),
    oRequest("oRequest", numBanks),
    iResponse("iResponse", numBanks),
    id(id),
    commandQueue(queueLength),
    memoryInterface("ifc", id, numBanks) {

  oRequest(memoryInterface.oRequest);
  iResponse(memoryInterface.iResponse);

  SC_METHOD(newCommandArrived);
  sensitive << iCommand;
  dont_initialize();

  SC_METHOD(detectIdleness);
  sensitive << memoryInterface.becameIdleEvent();
  dont_initialize();

}

void DMA::enqueueCommand(const dma_command_t command) {
  commandQueue.enqueue(command);
}

bool DMA::canAcceptCommand() const {
  return !commandQueue.full();
}

void DMA::replaceMemoryMapping(EncodedCMTEntry mapEncoded) {
  ChannelMapEntry::MemoryChannel decoded =
      ChannelMapEntry::memoryView(mapEncoded);

  // Update the memory channel so the memory knows which component to return
  // data to.
  decoded.channel = parent().memoryAccessChannel(id);

  memoryInterface.replaceMemoryMapping(decoded);
}

// Magic connection from memory.
void DMA::deliverDataInternal(const NetworkData& flit) {
  memoryInterface.receiveResponse(flit);
}

bool DMA::isIdle() const {
  return commandQueue.empty() && memoryInterface.isIdle();
}

const sc_event& DMA::becameIdleEvent() const {
  return becameIdle;
}

// Remove and return the next command from the control unit.
const dma_command_t DMA::dequeueCommand() {
  return commandQueue.dequeue();
}

// Generate a new memory request and send it to the appropriate memory bank.
void DMA::createNewRequest(position_t position, MemoryAddr address,
                           MemoryOpcode op, int payloadFlits, int data) {
  memoryInterface.createNewRequest(position, address, op, payloadFlits, data);
}

// Send a request to memory. If a response is expected, it will trigger the
// memoryInterface.responseArrivedEvent() event.
// TODO: instead of breaking down the request into individual accesses, pass
// more information to the memory interface and let it decide how access should
// work. It may prefer to do bulk reads/writes, for example.
void DMA::memoryAccess(position_t position, MemoryAddr address, MemoryOpcode op,
                       int data) {
  switch (op) {
    case LOAD_W:
      loadData(position, address);
      break;
    case STORE_W:
      storeData(position, address, data);
      break;
    case LOAD_AND_ADD:
      loadAndAdd(position, address, data);
      break;
    default:
      throw InvalidOptionException("Convolution memory operation", op);
      break;
  }
}

// Instantly send a request to memory. This is not a substitute for
// memoryAccess above: this method is only to be called once all details about
// the access have been confirmed.
void DMA::magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address,
                            ChannelID returnChannel, Word data) {
  parent().magicMemoryAccess(opcode, address, returnChannel, data);
}

Accelerator& DMA::parent() const {
  return static_cast<Accelerator&>(*(this->get_parent_object()));
}

void DMA::newCommandArrived() {
  loki_assert(canAcceptCommand());
  enqueueCommand(iCommand.read());
}

void DMA::detectIdleness() {
  if (isIdle()) {
    LOKI_LOG << this->name() << " is now idle" << endl;
    becameIdle.notify(sc_core::SC_ZERO_TIME);
  }
}


