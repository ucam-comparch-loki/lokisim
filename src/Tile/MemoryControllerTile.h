/*
 * MemoryControllerTile.h
 *
 * Tile with a connection to main memory.
 *
 *  Created on: 6 Oct 2016
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORYCONTROLLERTILE_H_
#define SRC_TILE_MEMORYCONTROLLERTILE_H_

#include "Tile.h"
#include "../Network/Global/NetworkDeadEnd.h"
#include "../Network/NetworkChannel.h"

class MemoryControllerTile: public Tile {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Tile:
//
//  ClockInput      clock;
//
//  // Data network.
//  sc_port<network_sink_ifc<Word>> iData;
//  sc_port<network_sink_ifc<Word>> oData;
//
//  // Credit network.
//  sc_port<network_sink_ifc<Word>> iCredit;
//  sc_port<network_sink_ifc<Word>> oCredit;
//
//  // Memory request network.
//  sc_port<network_sink_ifc<Word>> iRequest;
//  sc_port<network_sink_ifc<Word>> oRequest;
//
//  // Memory response network.
//  sc_port<network_sink_ifc<Word>> iResponse;
//  sc_port<network_sink_ifc<Word>> oResponse;

  // Extra interfaces to main memory.
  sc_port<network_sink_ifc<Word>>   oRequestToMainMemory;
  sc_port<network_source_ifc<Word>> iResponseFromMainMemory;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(MemoryControllerTile);
  MemoryControllerTile(const sc_module_name& name, const TileID id);
  virtual ~MemoryControllerTile();

//============================================================================//
// Methods
//============================================================================//

private:

  // Extra initialisation once all ports have been bound.
  void end_of_elaboration();

  // Forward requests on to main memory.
  void requestLoop();

  // Pass responses back to the on-chip network.
  void responseLoop();

//============================================================================//
// Local state
//============================================================================//

private:

  // Requests from on-chip.
  NetworkChannel<Word> incomingRequests;

  // This tile is not connected to the data or credit networks.
  NetworkDeadEnd<Word> dataDeadEnd;
  NetworkDeadEnd<Word> creditDeadEnd;

  // The tile is only connected to request inputs and response outputs.
  NetworkDeadEnd<Word> requestDeadEnd;
  NetworkDeadEnd<Word> responseDeadEnd;

};

#endif /* SRC_TILE_MEMORYCONTROLLERTILE_H_ */
