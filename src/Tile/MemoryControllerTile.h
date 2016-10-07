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
//  DataInput       iData;
//  DataOutput      oData;
//  ReadyInput      iDataReady;
//  ReadyOutput     oDataReady;
//
//  // Credit network.
//  CreditInput     iCredit;
//  CreditOutput    oCredit;
//  ReadyInput      iCreditReady;
//  ReadyOutput     oCreditReady;
//
//  // Memory request network.
//  RequestInput    iRequest;
//  RequestOutput   oRequest;
//  ReadyInput      iRequestReady;
//  ReadyOutput     oRequestReady;
//
//  // Memory response network.
//  ResponseInput   iResponse;
//  ResponseOutput  oResponse;
//  ReadyInput      iResponseReady;
//  ReadyOutput     oResponseReady;

  // Extra interfaces to main memory.
  RequestOutput   oRequestToMainMemory;
  ResponseInput   iResponseFromMainMemory;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(MemoryControllerTile);
  MemoryControllerTile(const sc_module_name& name, const ComponentID& id);
  virtual ~MemoryControllerTile();

//============================================================================//
// Methods
//============================================================================//

private:

  // Forward requests on to main memory.
  void requestLoop();

  // Pass responses back to the on-chip network.
  void responseLoop();

//============================================================================//
// Local state
//============================================================================//

private:

  // This tile is not connected to the data or credit networks.
  NetworkDeadEnd<NetworkData>     dataDeadEnd;
  NetworkDeadEnd<NetworkCredit>   creditDeadEnd;

  // The tile is only connected to request inputs and response outputs.
  // Merge the unused parts into a single dead end component.
  NetworkDeadEnd<NetworkResponse> responseDeadEnd;

};

#endif /* SRC_TILE_MEMORYCONTROLLERTILE_H_ */
