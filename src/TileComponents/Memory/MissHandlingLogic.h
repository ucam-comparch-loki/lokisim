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
  typedef loki_out<MemoryRequest> OutRequestPort;
  typedef loki_in<Word> InDataPort;
  typedef loki_out<Word> OutDataPort;

public:

  ClockInput                  clock;

  // One request per bank.
  LokiVector<InRequestPort>   iRequestFromBanks;

  // Forward the chosen request on to the next level of memory hierarchy.
  RequestOutput               oRequestToNetwork;

  // Responses from the next level of memory hierarchy.
  ResponseInput               iResponseFromNetwork;

  // Signal to the network that a response can be received.
  // This isn't actually required, since we ensure there is buffer space before
  // sending a request.
  ReadyOutput                 oReadyForResponse;

  // Responses are broadcast to all banks.
  OutDataPort                 oDataToBanks;


  // Magic connections to background memory.

  // Requests to background memory.
  OutRequestPort              oRequestToBM;

  // Data from background memory.
  InDataPort                  iDataFromBM;

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
  void handleStore();

  void handleEndOfRequest();


  // The following several methods allow the implementation of the next level
  // of memory hierarchy to be hidden. It could be a magic background memory,
  // or in a constant position on the chip, or spread over a number of L2 tiles.

  void sendOnNetwork(MemoryRequest request);
  bool canSendOnNetwork() const;
  const sc_event& canSendEvent() const;

  Word receiveFromNetwork();
  bool networkDataAvailable() const;
  const sc_event& newNetworkDataEvent() const;

  // The network address the request should be sent to.
  ChannelID getDestination(MemoryRequest request);

//==============================//
// Local state
//==============================//

private:

  enum MHLState {
    MHL_READY,      // Nothing to do
    MHL_FETCH,      // Fetching data from remote location
    MHL_STORE,      // Flushing data to remote location
  };

  MHLState state;

  // Multiplexer which selects an input from one of the connected banks.
  ArbitratedMultiplexer<MemoryRequest> inputMux;

  // Selected request from connected memory banks.
  loki_signal<MemoryRequest> muxOutput;

  // Keep the same multiplexer input if multiple flits are being sent.
  sc_signal<bool> holdMux;

  // Keep track of how many flits we expect to receive before we can move on
  // to the next task. This will typically be the length of a cache line.
  uint flitsRemaining;


};

#endif /* MISSHANDLINGLOGIC_H_ */
