/*
 * MemoryControllerTile.cpp
 *
 *  Created on: 6 Oct 2016
 *      Author: db434
 */

#include "MemoryControllerTile.h"
#include "../Utility/Assert.h"

MemoryControllerTile::MemoryControllerTile(const sc_module_name& name, const ComponentID& id) :
    Tile(name, id),
    dataDeadEnd("data", id, LOCAL),
    creditDeadEnd("credit", id, LOCAL),
    responseDeadEnd("response", id, LOCAL) {

  // Tie off unused networks here.
  dataDeadEnd.iData(iData);
  dataDeadEnd.oData(oData);
  dataDeadEnd.iReady(iDataReady);
  dataDeadEnd.oReady(oDataReady);

  creditDeadEnd.iData(iCredit);
  creditDeadEnd.oData(oCredit);
  creditDeadEnd.iReady(iCreditReady);
  creditDeadEnd.oReady(oCreditReady);

  responseDeadEnd.iData(iResponse);
  responseDeadEnd.oData(oRequest);
  responseDeadEnd.iReady(iRequestReady);
  responseDeadEnd.oReady(oResponseReady);

  oRequestReady.initialize(true);

  // Register methods to deal with input.
  SC_METHOD(requestLoop);
  sensitive << iRequest;
  dont_initialize();

  SC_METHOD(responseLoop);
  sensitive << iResponseFromMainMemory;
  dont_initialize();

}

MemoryControllerTile::~MemoryControllerTile() {
  // Nothing
}

void MemoryControllerTile::requestLoop() {
  if (oRequestToMainMemory.valid()) {
    next_trigger(oRequestToMainMemory.ack_event());
  }
  else if (iRequest.valid()) {
    oRequestToMainMemory.write(iRequest.read());
    iRequest.ack();

    LOKI_LOG << this->name() << " forwarding request to main memory: "
        << iRequest.read() << endl;

    // Default trigger: new request arrival
  }
}

void MemoryControllerTile::responseLoop() {
  if (!iResponseReady.read()) {
    next_trigger(iResponseReady.posedge_event());
  }
  else if (iResponseFromMainMemory.valid()) {
    loki_assert(!oResponse.valid());
    oResponse.write(iResponseFromMainMemory.read());

    iResponseFromMainMemory.ack();

    LOKI_LOG << this->name() << " forwarding response from main memory: "
        << iResponseFromMainMemory.read() << endl;

    // Wait for the clock edge rather than the next data arrival because there
    // may be other data lined up and ready to go immediately.
    next_trigger(clock.posedge_event());
  }
  // else default trigger: new data to send
}
