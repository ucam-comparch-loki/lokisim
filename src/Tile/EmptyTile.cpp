/*
 * EmptyTile.cpp
 *
 *  Created on: 5 Oct 2016
 *      Author: db434
 */

#include "EmptyTile.h"

EmptyTile::EmptyTile(const sc_module_name& name, const TileID& id) :
    Tile(name, id),
    dataDeadEnd("data", id, "local"),
    creditDeadEnd("credit", id, "local"),
    requestDeadEnd("request", id, "local"),
    responseDeadEnd("response", id, "local") {

  // Connect everything up.
  iData(dataDeadEnd);
//  oData(dataDeadEnd);

  iCredit(creditDeadEnd);
//  oCredit(creditDeadEnd);

  iRequest(requestDeadEnd);
//  oRequest(requestDeadEnd);

  iResponse(responseDeadEnd);
//  oResponse(responseDeadEnd);

}

EmptyTile::~EmptyTile() {
  // Nothing
}

