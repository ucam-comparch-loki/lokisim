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

#include "Component.h"

class MissHandlingLogic: public Component {

//==============================//
// Ports
//==============================//

public:

  ClockInput                  clock;

  LokiVector<RequestInput>    iRequestFromBanks;
  RequestOutput               oRequestToRouter;

  ResponseInput               iResponseFromRouter;
  LokiVector<ResponseOutput>  oResponseToBanks;

//==============================//
// Constructors and destructors
//==============================//

public:

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


};

#endif /* MISSHANDLINGLOGIC_H_ */
