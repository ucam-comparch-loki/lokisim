/*
 * MemoryInterface.h
 *
 * Component responsible for getting data to or from memory for an Accelerator's
 * DMA unit.
 *
 * This component is agnostic to the type of data being accessed, and always
 * returns uint32_t values. These should be cast to the appropriate type before
 * use.
 *
 *  Created on: 10 Aug 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_MEMORYINTERFACE_H_
#define SRC_TILE_ACCELERATOR_MEMORYINTERFACE_H_

#include <queue>
#include "../../LokiComponent.h"
#include "../../Memory/MemoryTypes.h"
#include "../../Network/NetworkTypes.h"
#include "../ChannelMapEntry.h"
#include "../MemoryBankSelector.h"

class DMA;
using std::queue;

// Position in array of ports to return a result.
typedef struct {
  uint row;
  uint column;
} position_t;

// Requests to send to memory.
typedef struct {
  position_t position;
  MemoryAddr address;
  MemoryOpcode operation;
  uint32_t   data;
} request_t;

// Value returned from memory, combined with its position.
// Note that data is always a uint32_t, and should be cast to the appropriate
// type before use.
typedef struct {
  position_t position;
  uint32_t data;
} response_t;

class MemoryInterface : public LokiComponent {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(MemoryInterface);
  MemoryInterface(sc_module_name name, ComponentID id);


//============================================================================//
// Methods
//============================================================================//

public:

  // Enqueue a memory request to be sent when next possible.
  void createNewRequest(position_t position, MemoryAddr address, MemoryOpcode op,
                        uint32_t data=0);

  // Dequeue the next value received from memory.
  const response_t getResponse();

  // Does this component have capacity to enqueue another memory request?
  bool canAcceptRequest() const;

  // Does this component have capacity to receive another response from memory?
  bool canAcceptResponse() const;

  // Does this component have a value ready to be passed on to the PEs?
  bool canGiveResponse() const;

  // Receive a response from memory.
  void receiveResponse(const NetworkData& flit);

  // Event triggered whenever there is a new response ready to send to the PEs.
  const sc_event& responseArrivedEvent() const;

  // This component is idle if it has no waiting requests, no waiting responses,
  // and there are no requests currently in flight in the memory system.
  bool isIdle() const;

  // Update the mapping from memory addresses to memory banks.
  void replaceMemoryMapping(ChannelMapEntry::MemoryChannel mapping);

private:

  // Send the next request in the request queue to memory.
  void sendRequest();

  DMA* parent() const;


//============================================================================//
// Local state
//============================================================================//

private:

  // Memory configuration. Tells us which memory bank to access for each memory
  // address.
  ChannelMapEntry::MemoryChannel memoryMapping;

  // Fine-tunes the memory configuration for individual requests if the mapping
  // covers multiple components.
  MemoryBankSelector bankSelector;

  // Requests to send to memory. Could potentially split this into one queue per
  // bank to increase parallelism.
  queue<request_t> requests;
  sc_event requestArrived;

  // Responses received from memory.
  queue<response_t> responses;
  sc_event responseArrived;

  // Probably temporary. Hold the PE position associated with each request while
  // we wait for a response from memory. Assumes that responses will arrive in
  // the same order as requests, which is only true for magic memory, or if huge
  // numbers of channels are available.
  queue<position_t> inFlight;

  // Keep track of the number of requests in progress so we know when they're all
  // done. A request begins when it first arrives in the request queue, and it
  // ends when its response is removed from the response queue.
  uint outstandingRequests;

};

#endif /* SRC_TILE_ACCELERATOR_MEMORYINTERFACE_H_ */
