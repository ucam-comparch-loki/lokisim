/*
 * DMA.cpp
 *
 *  Created on: 21 Aug 2018
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

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
    memoryMapping(-1),
    bankSelector(id) {

  currentTick = 0;

  for (uint i=0; i<numBanks; i++) {
    MemoryInterface* ifc = new MemoryInterface(sc_gen_unique_name("ifc"), i);
    oRequest[i](ifc->oRequest);
    iResponse[i](ifc->iResponse);

    SPAWN_METHOD(ifc->responseArrivedEvent(), DMA::receiveMemoryData, i, false);

    memoryInterfaces.push_back(ifc);
  }

  SC_METHOD(newCommandArrived);
  sensitive << iCommand;
  dont_initialize();

  SC_METHOD(detectIdleness);
  for (uint i=0; i<memoryInterfaces.size(); i++)
    sensitive << memoryInterfaces[i].becameIdleEvent();
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

  memoryMapping = decoded;
}

// Magic connection from memory.
void DMA::deliverDataInternal(const NetworkData& flit) {
  uint target = flit.channelID().channel;
  memoryInterfaces[target].receiveResponse(flit);
}

bool DMA::isIdle() const {
  if (!commandQueue.empty())
    return false;

  for (uint i=0; i<memoryInterfaces.size(); i++)
    if (!memoryInterfaces[i].isIdle())
      return false;

  return true;
}

const sc_event& DMA::becameIdleEvent() const {
  return becameIdle;
}

// Remove and return the next command from the control unit.
const dma_command_t DMA::dequeueCommand() {
  return commandQueue.dequeue();
}

// Generate a new memory request and send it to the appropriate memory bank.
void DMA::createNewRequest(tick_t tick, position_t position, MemoryAddr address,
                           MemoryOpcode op, int payloadFlits, int data) {
  // TODO: Create a MemoryOperation here.

  // Determine which memory interface to use.
  ChannelID memoryBank = bankSelector.getMapping(op, address, memoryMapping);
  ComponentIndex bankID = memoryBank.component.position;

  memoryInterfaces[bankID].createNewRequest(tick, position, address, op,
                                            payloadFlits, data);
}

// Send a request to memory. If a response is expected, it will trigger the
// memoryInterface.responseArrivedEvent() event.
// TODO: instead of breaking down the request into individual accesses, pass
// more information to the memory interface and let it decide how access should
// work. It may prefer to do bulk reads/writes, for example.
void DMA::memoryAccess(tick_t tick, position_t position, MemoryAddr address,
                       MemoryOpcode op, int data) {
  switch (op) {
    case LOAD_W:
      loadData(tick, position, address);
      break;
    case STORE_W:
      storeData(tick, position, address, data);
      break;
    case LOAD_AND_ADD:
      loadAndAdd(tick, position, address, data);
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
    LOKI_LOG(3) << this->name() << " is now idle" << endl;
    becameIdle.notify(sc_core::SC_ZERO_TIME);
  }
}


