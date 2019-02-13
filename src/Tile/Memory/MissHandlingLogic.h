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

#include "../../Datatype/MemoryRequest.h"
#include "../../LokiComponent.h"
#include "../../Network/ArbitratedMultiplexer.h"
#include "../../Network/NetworkTypes.h"
#include "../../Tile/Memory/Directory.h"
#include "../../Utility/LokiVector.h"

class Chip;
class ComputeTile;

class MissHandlingLogic: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput                  clock;


  // Ports to handle requests from memory banks on this tile.

  // One request per bank.
  LokiVector<RequestInput>    iRequestFromBanks;

  // Forward the chosen request on to the next level of memory hierarchy.
  RequestOutput               oRequestToNetwork;

  // Responses from the next level of memory hierarchy.
  ResponseInput               iResponseFromNetwork;

  // Signal to the network that a response can be received.
  ReadyOutput                 oReadyForResponse;

  // Responses are broadcast to all banks.
  ResponseOutput              oResponseToBanks;

  // The memory bank for which this response is meant.
  sc_out<MemoryIndex>         oResponseTarget;


  // Ports to handle requests from memory banks on other tiles.

  // Receive responses from memory banks.
  LokiVector<ResponseInput>   iResponseFromBanks;

  // Send responses back to the requesting tile.
  ResponseOutput              oResponseToNetwork;

  // Requests from memory banks on other tiles.
  RequestInput                iRequestFromNetwork;

  // Signal whether we're able to receive another request at this time.
  ReadyOutput                 oReadyForRequest;

  // Broadcast requests to all banks on the tile.
  RequestOutput               oRequestToBanks;

  // If no bank is able to respond to the request, the target bank is the one
  // which should assume responsibility.
  sc_out<MemoryIndex>         oRequestTarget;

  // Signal from each bank telling whether it has claimed the latest request.
  LokiVector<sc_in<bool> >    iClaimRequest;

  // Signal from each bank telling demanding that the current request be
  // delayed. This may be because the bank is already processing another
  // request which interferes with this one.
  LokiVector<sc_in<bool> >    iDelayRequest;

  // Signal broadcast to all banks, telling whether the request has been claimed.
  sc_out<bool>                oRequestClaimed;

  // Signal broadcast to all banks, telling whether the request has been delayed.
  sc_out<bool>                oRequestDelayed;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(MissHandlingLogic);
  MissHandlingLogic(const sc_module_name& name,
                    const tile_parameters_t& params);

//============================================================================//
// Methods
//============================================================================//

public:

  // Determine whether the given address is a cached copy of data from main
  // memory.
  bool backedByMainMemory(MemoryAddr address) const;

  // Determine the address in main memory represented by the given address.
  MemoryAddr getAddressTranslation(MemoryAddr address) const;

private:

  // Process requests from the local memory banks.
  void localRequestLoop();
  void handleDirectoryUpdate();
  void handleDirectoryMaskUpdate();

  // Receive responses from remote memory banks.
  void receiveResponseLoop();

  // Process requests from remote memory banks.
  void remoteRequestLoop();

  // Update whether the remote request has been claimed by any local bank.
  void requestClaimLoop();

  // Update whether the remote request has been delayed by any local bank.
  void requestDelayLoop();

  // Send responses to remote memory banks.
  void sendResponseLoop();

  // Determine which memory bank should be the target for the given request.
  MemoryIndex getTargetBank(const NetworkRequest& request);

  // Pseudo-randomly select a target bank.
  MemoryIndex nextRandomBank();

  void sendOnNetwork(NetworkRequest request);
  bool canSendOnNetwork() const;
  const sc_event& canSendEvent() const;

  NetworkResponse receiveFromNetwork();
  bool networkDataAvailable() const;
  const sc_event& newNetworkDataEvent() const;

  // The network address of the memory controller.
  TileID nearestMemoryController() const;

  // A reference to the parent tile.
  ComputeTile& tile() const;

  // A reference to the parent chip.
  Chip& chip() const;

//============================================================================//
// Local state
//============================================================================//

private:

  // Configuration.
  const size_t log2CacheLineSize; // In bytes.
  const size_t numMemoryBanks;

  friend class ComputeTile;

  // Mapping between memory addresses and home tiles.
  Directory directory;

  // Multiplexer which selects an input from one of the connected banks.
  ArbitratedMultiplexer<NetworkRequest>  requestMux;
  ArbitratedMultiplexer<NetworkResponse> responseMux;

  // Selected request from connected memory banks.
  loki_signal<NetworkRequest>  muxedRequest;
  loki_signal<NetworkResponse> muxedResponse;

  // Keep the same multiplexer input if multiple flits are being sent.
  sc_signal<bool> holdRequestMux, holdResponseMux;

  // Flag telling whether the next flit to arrive will be the start of a new
  // packet.
  bool newLocalRequest, newRemoteRequest;

  // The network address to which the current packet should be sent.
  ChannelID requestDestination;

  // The first flit of a request which is to take place at the directory.
  NetworkRequest requestHeader;
  bool requestHeaderValid;

  // Used for random number generation.
  unsigned int rngState;


};

#endif /* MISSHANDLINGLOGIC_H_ */
