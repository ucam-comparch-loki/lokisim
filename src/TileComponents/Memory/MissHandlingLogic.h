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
#include "Directory.h"

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


  // Ports to handle requests from memory banks on this tile.

  // One request per bank.
  LokiVector<InRequestPort>   iRequestFromBanks;

  // Forward the chosen request on to the next level of memory hierarchy.
  RequestOutput               oRequestToNetwork;

  // Responses from the next level of memory hierarchy.
  ResponseInput               iResponseFromNetwork;

  // Signal to the network that a response can be received.
  ReadyOutput                 oReadyForResponse;

  // Responses are broadcast to all banks.
  OutDataPort                 oDataToBanks;

  // The memory bank for which this response is meant.
  sc_out<MemoryIndex>         oResponseTarget;


  // Ports to handle requests from memory banks on other tiles.

  // Receive responses from memory banks.
  LokiVector<InDataPort>      iDataFromBanks;

  // Send responses back to the requesting tile.
  ResponseOutput              oResponseToNetwork;

  // Requests from memory banks on other tiles.
  RequestInput                iRequestFromNetwork;

  // Signal whether we're able to receive another request at this time.
  ReadyOutput                 oReadyForRequest;

  // Broadcast requests to all banks on the tile.
  OutRequestPort              oRequestToBanks;

  // If no bank is able to respond to the request, the target bank is the one
  // which should assume responsibility.
  sc_out<MemoryIndex>         oRequestTarget;


  // Magic connections to background memory.

  // Requests to background memory.
  OutRequestPort              oRequestToBM;

  // Data from background memory.
  ResponseInput               iResponseFromBM;

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

  // Process requests from the local memory banks.
  void localRequestLoop();

  void handleNewLocalRequest();
  void handleLocalFetch();
  void handleLocalStore();
  void handleDirectoryUpdate();
  void handleDirectoryMaskUpdate();
  void endLocalRequest();

  void receiveResponseLoop();

  // Process requests from remote memory banks.
  void remoteRequestLoop();

  void handleNewRemoteRequest();
  void handleRemoteFetch();
  void handleRemoteStore();
  void endRemoteRequest();

  // The following several methods allow the implementation of the next level
  // of memory hierarchy to be hidden. It could be a magic background memory,
  // or in a constant position on the chip, or spread over a number of L2 tiles.

  void sendOnNetwork(MemoryRequest request, bool endOfPacket);
  bool canSendOnNetwork() const;
  const sc_event& canSendEvent() const;

  NetworkResponse receiveFromNetwork();
  bool networkDataAvailable() const;
  const sc_event& newNetworkDataEvent() const;

  // The network address the request should be sent to.
  ChannelID getDestination(MemoryAddr address) const;

  // The network address of the memory controller.
  ChannelID memoryControllerAddress() const;

//==============================//
// Local state
//==============================//

private:

  enum MHLState {
    MHL_READY,      // Nothing to do
    MHL_FETCH,      // Fetching cache line
    MHL_STORE,      // Flushing cache line
  };

  // Have a loop each for requests from local memory banks for data elsewhere,
  // and from remote memory banks for data stored on this tile.
  MHLState localState, remoteState;

  // Mapping between memory addresses and home tiles.
  Directory directory;

  // Multiplexer which selects an input from one of the connected banks.
  ArbitratedMultiplexer<MemoryRequest> requestMux;
  ArbitratedMultiplexer<Word>          responseMux;

  // Selected request from connected memory banks.
  loki_signal<MemoryRequest> muxedRequest;
  loki_signal<Word>          muxedResponse;

  // Keep the same multiplexer input if multiple flits are being sent.
  sc_signal<bool> holdRequestMux, holdResponseMux;

  // The network address to which the current packet should be sent.
  ChannelID requestDestination, responseDestination;

  // Keep track of how many flits we expect to receive before we can move on
  // to the next task. This will typically be the length of a cache line.
  uint requestFlitsRemaining, responseFlitsRemaining;


};

#endif /* MISSHANDLINGLOGIC_H_ */
