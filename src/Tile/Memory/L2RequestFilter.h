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
#include "../../Network/FIFOs/NetworkFIFO.h"
#include "../../Network/NetworkTypes.h"
#include "../Network/L2LToBankRequests.h"

class MemoryBank;

class L2RequestFilter: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput            iClock;           // Clock

  sc_port<network_sink_ifc<Word>> iRequest; // Input requests from the network
  RequestOutput         oRequest;         // Requests for this bank to serve

  sc_port<l2_request_bank_ifc> l2Associativity; // Coordination with other banks

//============================================================================//
// Internal functions
//============================================================================//

public:
  SC_HAS_PROCESS(L2RequestFilter);
  L2RequestFilter(const sc_module_name& name, MemoryBank& localBank);
  virtual ~L2RequestFilter();

protected:

  virtual void end_of_elaboration();

private:

  // Check whether requests should be accepted by this memory bank.
  void mainLoop();

//============================================================================//
// Local state
//============================================================================//

private:

  enum FilterState {
    STATE_IDLE,       // No request/current request is for another bank
    STATE_WAIT,       // Wait to see if any other bank can serve the request
    STATE_SEND,       // Send a request flit on to the local bank
  };
  FilterState state;

  const MemoryBank& localBank;

  NetworkFIFO<Word> inBuffer;

};

#endif /* SRC_TILE_MEMORY_L2REQUESTFILTER_H_ */
