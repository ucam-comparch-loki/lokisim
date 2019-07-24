/*
 * MemoryInterface.h
 *
 * Component responsible for getting data to or from memory for an Accelerator's
 * DMA unit. One memory interface accesses exactly one memory bank, so all
 * requests are processed in order.
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
#include "../../Network/FIFOs/NetworkFIFO.h"
#include "../../Network/Interface.h"
#include "../../Network/NetworkTypes.h"
#include "../../Utility/BlockingInterface.h"
#include "../../Utility/LokiVector.h"
#include "../ChannelMapEntry.h"
#include "../MemoryBankSelector.h"
#include "AcceleratorTypes.h"

class DMA;
using std::queue;

// Position in array of ports to return a result.
class position_t {
public:
  uint row;
  uint column;

  friend std::ostream& operator<< (std::ostream& os, position_t const& p) {
    os << "(" << p.row << ", " << p.column << ")";
    return os;
  }
};

// Requests to send to memory.
typedef struct {
  tick_t     tick;
  position_t position;
  MemoryAddr address;
  MemoryOpcode operation;
  bool       hasPayload;
  uint32_t   data;
} request_t;

// Value returned from memory, combined with its position.
// Note that data is always a uint32_t, and should be cast to the appropriate
// type before use.
typedef struct {
  tick_t   tick;
  position_t position;
  uint32_t data;
} response_t;

class MemoryInterface : public LokiComponent, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:

  typedef sc_port<network_sink_ifc<Word>> InPort;
  typedef sc_port<network_source_ifc<Word>> OutPort;

  OutPort oRequest;   // Requests sent to memory.
  InPort  iResponse;  // Responses from memory.

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(MemoryInterface);
  MemoryInterface(sc_module_name name, ComponentIndex memory);


//============================================================================//
// Methods
//============================================================================//

public:

  // Enqueue a memory request to be sent when next possible.
  void createNewRequest(tick_t tick, position_t position, MemoryAddr address,
                        MemoryOpcode op, uint payloadFlits, uint32_t data=0);

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
  const sc_event& becameIdleEvent() const;

protected:

  virtual void reportStalls(ostream& os);

private:

  // Send the next request in the request queue to memory.
  void sendRequest();

  // Take a response from memory and reconnect it with any relevant metadata,
  // before passing it back to the DMA.
  void processResponse();

  DMA& parent() const;


//============================================================================//
// Local state
//============================================================================//

private:

  // The index of the memory bank to be accessed.
  const ComponentIndex memory;

  // Requests to send to memory.
  queue<request_t> requests;
  sc_event requestArrived;
  NetworkFIFO<Word> requestBuffer;

  // Responses received from memory.
  queue<response_t> responses;
  sc_event responseArrived;
  NetworkFIFO<Word> responseBuffer;

  // Hold the PE position associated with each request while we wait for a
  // response from memory. Assumes that responses will arrive in the same order
  // as requests.
  queue<request_t> inFlight;

  // Event triggered when the final request has completed and its result (if
  // any) has been consumed.
  sc_event becameIdle;

};

#endif /* SRC_TILE_ACCELERATOR_MEMORYINTERFACE_H_ */
