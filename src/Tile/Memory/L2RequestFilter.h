/*
 * L2RequestFilter.h
 *
 * Monitor the memory requests broadcast to all memory banks in a tile, and
 * filter out the ones which should be served by this bank.
 *
 *  Created on: 6 Jul 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_L2REQUESTFILTER_H_
#define SRC_TILE_MEMORY_L2REQUESTFILTER_H_

#include "../../LokiComponent.h"
#include "../../Network/NetworkTypes.h"

class MemoryBank;

class L2RequestFilter: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput            iClock;           // Clock

  RequestInput          iRequest;         // Input requests sent to the memory bank
  sc_in<MemoryIndex>    iRequestTarget;   // The responsible bank if all banks miss
  RequestOutput         oRequest;         // Requests for this bank to serve

  sc_in<bool>           iRequestClaimed;  // One of the banks has claimed the request.
  sc_out<bool>          oClaimRequest;    // Tell whether this bank has claimed the request.

  sc_in<bool>           iRequestDelayed;  // One of the banks has delayed the request.
  sc_out<bool>          oDelayRequest;    // Block other banks from processing the request.

//============================================================================//
// Internal functions
//============================================================================//

public:
  SC_HAS_PROCESS(L2RequestFilter);
  L2RequestFilter(const sc_module_name& name, ComponentID id, MemoryBank* localBank);
  virtual ~L2RequestFilter();

private:

  // Check whether requests should be accepted by this memory bank.
  void mainLoop();

  // Pass requests on to the memory bank.
  void forwardToMemoryBank(NetworkRequest request);

  // Monitor whether it is safe for other banks to proceed with the current
  // request. It may not be safe if this bank is currently doing something which
  // may interfere (e.g. flushing some data which is about to be read).
  void delayLoop();

//============================================================================//
// Local state
//============================================================================//

private:

  enum FilterState {
    STATE_IDLE,       // No request/current request is for another bank
    STATE_WAIT,       // Wait to see if any other bank can serve the request
    STATE_SEND,       // Send a request flit on to the local bank
    STATE_ACKNOWLEDGE // Acknowledge the received flit when the bank consumes it
  };
  FilterState state;

  const MemoryBank*     localBank;


};

#endif /* SRC_TILE_MEMORY_L2REQUESTFILTER_H_ */
