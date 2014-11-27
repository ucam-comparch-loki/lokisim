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

  typedef loki_out<Word> OutDataPort;

public:

  ClockInput                  clock;

  // One request per bank.
  LokiVector<RequestInput>    iRequestFromBanks;

  // Forward the chosen request on to the next level of memory hierarchy.
  RequestOutput               oRequestToNetwork;

  // Responses from the next level of memory hierarchy.
  ResponseInput               iResponseFromNetwork;

  // Responses are broadcast to all banks.
  OutDataPort                 oDataToBanks;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(MissHandlingLogic);
  MissHandlingLogic(const sc_module_name& name, ComponentID id);

//==============================//
// Methods
//==============================//

private:

  void mainLoop();

  void handleNewRequest();
  void handleFetch();
  void handleWriteBack();
  void handleAllocate();
  void handleAllocateHit();

  void handleEndOfRequest();


  // The following several methods allow the implementation of the next level
  // of memory hierarchy to be hidden. It could be a magic background memory,
  // or in a constant position on the chip, or spread over a number of L2 tiles.

  void sendOnNetwork(MemoryRequest request);
  bool canSendOnNetwork() const;
  sc_event canSendEvent() const;

  NetworkResponse receiveFromNetwork();
  bool networkDataAvailable() const;
  sc_event newNetworkDataEvent() const;

  // The network address the request should be sent to.
  ChannelID getDestination(MemoryRequest request);

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

  // Multiplexer which selects an input from one of the connected banks.
  ArbitratedMultiplexer<NetworkRequest> inputMux;

  // Selected request from connected memory banks.
  loki_signal<NetworkRequest> muxOutput;

  // Keep the same multiplexer input if multiple flits are being sent.
  sc_signal<bool> holdMux;


};

#endif /* MISSHANDLINGLOGIC_H_ */
