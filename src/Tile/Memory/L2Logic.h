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
#include "../../Network/FIFOs/NetworkFIFO.h"
#include "../../Utility/LokiVector.h"
#include "../Network/BankAssociation.h"

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
  InPort                      iResponseFromBanks;

  // Broadcast requests to all banks on the tile.
  sc_port<network_source_ifc<Word>> oRequestToBanks;

  // Interface for determining which bank is responsible for each request.
  sc_port<association_l2l_ifc> associativity;

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

  // Update the target bank output whenever a new request is sent to the banks.
  void updateTargetBank();

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

  // Used for random number generation.
  unsigned int rngState;
  MemoryIndex randomBank;
};

#endif /* SRC_TILE_MEMORY_L2LOGIC_H_ */
