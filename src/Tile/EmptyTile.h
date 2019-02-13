/*
 * EmptyTile.h
 *
 * A tile on the edge of the chip with no internal logic.
 *
 * Any data arriving here over the on-chip network will trigger an error.
 *
 *  Created on: 5 Oct 2016
 *      Author: db434
 */

#ifndef SRC_TILE_EMPTYTILE_H_
#define SRC_TILE_EMPTYTILE_H_

#include "Tile.h"
#include "../Network/Global/NetworkDeadEnd.h"

class EmptyTile: public Tile {

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

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  EmptyTile(const sc_module_name& name, const TileID& id);
  virtual ~EmptyTile();

//============================================================================//
// Local state
//============================================================================//

private:

  NetworkDeadEnd<NetworkData>     dataDeadEnd;
  NetworkDeadEnd<NetworkCredit>   creditDeadEnd;
  NetworkDeadEnd<NetworkRequest>  requestDeadEnd;
  NetworkDeadEnd<NetworkResponse> responseDeadEnd;

};

#endif /* SRC_TILE_EMPTYTILE_H_ */
