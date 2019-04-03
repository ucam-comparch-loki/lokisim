/*
 * L2Logic.h
 *
 * Logic which allows the memory banks of a single tile to behave as a unified
 * associative cache.
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_L2LOGIC_H_
#define SRC_TILE_MEMORY_L2LOGIC_H_

#include "../../LokiComponent.h"
#include "../../Network/ArbitratedMultiplexer.h"
#include "../../Network/FIFOs/NetworkFIFO.h"
#include "../../Utility/LokiVector.h"

class L2Logic: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

  // Outputs go straight to a router which has its own buffer.
  // Inputs need to be latched locally.
  typedef sc_port<network_sink_ifc<Word>> InPort;
  typedef sc_port<network_sink_ifc<Word>> OutPort;

public:

  ClockInput                  clock;

  // Connections to the global networks.

  // Requests from memory banks on other tiles.
  InPort                      iRequestFromNetwork;

  // Send responses back to the requesting tile.
  OutPort                     oResponseToNetwork;


  // Ports to handle requests from memory banks on other tiles.

  // Receive responses from memory banks.
  LokiVector<ResponseInput>   iResponseFromBanks;

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

  SC_HAS_PROCESS(L2Logic);
  L2Logic(const sc_module_name& name, const tile_parameters_t& params);

//============================================================================//
// Methods
//============================================================================//

private:

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

//============================================================================//
// Local state
//============================================================================//

private:

  // Configuration.
  const size_t log2CacheLineSize; // In bytes.
  const size_t numMemoryBanks;

  // Buffers/latches for network communications.
  NetworkFIFO<Word> requestsFromNetwork;

  // Flag telling whether the next flit to arrive will be the start of a new
  // packet.
  bool newRemoteRequest;

  // Multiplexer which selects an input from one of the connected banks.
  ArbitratedMultiplexer<NetworkResponse> responseMux;

  // Selected request from connected memory banks.
  loki_signal<NetworkResponse> muxedResponse;

  // Keep the same multiplexer input if multiple flits are being sent.
  sc_signal<bool> holdResponseMux;

  // Used for random number generation.
  unsigned int rngState;
};

#endif /* SRC_TILE_MEMORY_L2LOGIC_H_ */
