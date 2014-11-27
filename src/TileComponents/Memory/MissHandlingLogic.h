/*
 * MissHandlingLogic.h
 *
 * Logic which deals with cache misses within a tile. Tasks include:
 *  * Arbitrating between the memory banks
 *  * Directing flushed data to the appropriate location
 *  * Requesting new data from the appropriate location
 *
 *  Created on: 8 Oct 2014
 *      Author: db434
 */

#ifndef MISSHANDLINGLOGIC_H_
#define MISSHANDLINGLOGIC_H_

#include "../../Component.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Network/ArbitratedMultiplexer.h"
#include "../../Network/NetworkTypedefs.h"

class MissHandlingLogic: public Component {

//==============================//
// Ports
//==============================//

  typedef loki_in<MemoryRequest> InRequestPort;

public:

  ClockInput                  clock;

  // One request per bank.
  LokiVector<InRequestPort>   iRequestFromBanks;

  // Forward the chosen request on to the next level of memory hierarchy.
  RequestOutput               oRequestToRouter;

  // Responses from the next level of memory hierarchy.
  ResponseInput               iResponseFromRouter;

  // Responses are broadcast to all banks.
  ResponseOutput              oResponseToBanks;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(MissHandlingLogic);
  MissHandlingLogic(const sc_module_name& name, ComponentID id);

//==============================//
// Local state
//==============================//

private:

  // Are these states for the memory banks instead?
  enum MHLState {
    MHL_READY,      // Nothing to do
    MHL_FETCH,      // Fetching instructions from remote location
    MHL_WB,         // Flushing data to remote location
    MHL_ALLOC,      // Retrieving data from remote location
    MHL_ALLOCHIT    // Finished retrieving data - serve request
  };

  MHLState state;

  ArbitratedMultiplexer<MemoryRequest> inputMux;
  loki_signal<MemoryRequest> muxOutput;


};

#endif /* MISSHANDLINGLOGIC_H_ */
