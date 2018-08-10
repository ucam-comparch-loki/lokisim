/*
 * MemoryInterface.h
 *
 *  Created on: 10 Aug 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_MEMORYINTERFACE_H_
#define SRC_TILE_ACCELERATOR_MEMORYINTERFACE_H_

#include <queue>
#include <systemc>

using sc_core::sc_event;
using std::queue;

// Requests to send to memory.
typedef Flit<Word> request_t;

// Position in array of ports to return a result.
typedef struct {
  uint row;
  uint column;
} position_t;

// Value returned from memory, combined with its position.
typedef struct {
  position_t position;
  uint32_t data;
} response_t;

class MemoryInterface {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  MemoryInterface();


//============================================================================//
// Methods
//============================================================================//

public:

  // Enqueue a memory request to be sent when next possible. If a request
  // consists of multiple flits, they cannot be interleaved with other requests.
  void sendRequest(const request_t request);

  // Dequeue the next value received from memory.
  const response_t getResponse();

  // Does this component have capacity to enqueue another memory request?
  bool canAcceptRequest() const;

  // Does this component have capacity to receive another response from memory?
  bool canAcceptResponse() const;

  // Does this component have a value ready to be passed on to the PEs?
  bool canGiveResponse() const;

  // Event triggered whenever there is a new response ready to send to the PEs.
  sc_event responseArrivedEvent() const;

  // This component is idle if it has no waiting requests, no waiting responses,
  // and there are no requests currently in flight in the memory system.
  bool isIdle() const;

private:

  // Send the next requests in the request queue to memory.
  void requestsToMemory();

  // Receive a response from memory.
  void receiveResponse(/*TODO params*/);


//============================================================================//
// Local state
//============================================================================//

private:

  // Requests to send to memory. Could potentially split this into one queue per
  // bank to increase parallelism.
  queue<request_t> requests;
  sc_event requestArrived;

  // Responses received from memory.
  queue<response_t> responses;
  sc_event responseArrived;

  // Keep track of the number of requests in progress so we know when they're all
  // done. A request begins when it first arrives in the request queue, and it
  // ends when its response is removed from the response queue.
  uint outstandingRequests;

};

#endif /* SRC_TILE_ACCELERATOR_MEMORYINTERFACE_H_ */
