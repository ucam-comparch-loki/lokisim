/*
 * EmptyTile.cpp
 *
 *  Created on: 5 Oct 2016
 *      Author: db434
 */

#include "EmptyTile.h"

EmptyTile::EmptyTile(const sc_module_name& name, const ComponentID& id) :
    Tile(name, id),
    dataDeadEnd("data", id, LOCAL),
    creditDeadEnd("credit", id, LOCAL),
    requestDeadEnd("request", id, LOCAL),
    responseDeadEnd("response", id, LOCAL) {

  // Connect everything up.
  dataDeadEnd.iData(iData);
  dataDeadEnd.oData(oData);
  dataDeadEnd.iReady(iDataReady);
  dataDeadEnd.oReady(oDataReady);

  creditDeadEnd.iData(iCredit);
  creditDeadEnd.oData(oCredit);
  creditDeadEnd.iReady(iCreditReady);
  creditDeadEnd.oReady(oCreditReady);

  requestDeadEnd.iData(iRequest);
  requestDeadEnd.oData(oRequest);
  requestDeadEnd.iReady(iRequestReady);
  requestDeadEnd.oReady(oRequestReady);

  responseDeadEnd.iData(iResponse);
  responseDeadEnd.oData(oResponse);
  responseDeadEnd.iReady(iResponseReady);
  responseDeadEnd.oReady(oResponseReady);

}

EmptyTile::~EmptyTile() {
  // Nothing
}

