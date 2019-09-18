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

#include "../../LokiComponent.h"
#include "../../Network/FIFOs/NetworkFIFO.h"
#include "../../Network/NetworkChannel.h"
#include "../../Network/NetworkTypes.h"
#include "../../Tile/Memory/Directory.h"

class Chip;
class ComputeTile;

class MissHandlingLogic: public LokiComponent {

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

  // Forward the chosen request on to the next level of memory hierarchy.
  OutPort                     oRequestToNetwork;

  // Responses from the next level of memory hierarchy.
  InPort                      iResponseFromNetwork;


  // Ports to handle requests from memory banks on this tile.

  // Requests from banks.
  InPort                      iRequestFromBanks;

  // Responses are broadcast to all banks.
  sc_port<network_source_ifc<Word>> oResponseToBanks;

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
  void handleDirectoryUpdate(const NetworkRequest& flit);
  void handleDirectoryMaskUpdate(const NetworkRequest& flit);

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

  friend class ComputeTile;

  // Mapping between memory addresses and home tiles.
  Directory directory;

  // Buffer/latches to hold requests and responses from the network.
  NetworkChannel<Word> requestsFromBanks;
  NetworkFIFO<Word> responsesFromNetwork;

  // Flag telling whether the next flit to arrive will be the start of a new
  // packet.
  bool newLocalRequest;

  // The network address to which the current packet should be sent.
  ChannelID requestDestination;

  // The first flit of a request which is to take place at the directory.
  NetworkRequest requestHeader;
  bool requestHeaderValid;


};

#endif /* MISSHANDLINGLOGIC_H_ */
