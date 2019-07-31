/*
 * MemoryControllerTile.cpp
 *
 *  Created on: 6 Oct 2016
 *      Author: db434
 */

#include "MemoryControllerTile.h"
#include "../Utility/Assert.h"
#include "../Utility/Instrumentation/Network.h"

MemoryControllerTile::MemoryControllerTile(const sc_module_name& name, const TileID id) :
    Tile(name, id),
    oRequestToMainMemory("oRequestToMainMemory"),
    iResponseFromMainMemory("iResponseFromMainMemory"),
    incomingRequests("request_buf", 4), // Not sure what size is sensible here
    dataDeadEnd("data", id, "local"),
    creditDeadEnd("credit", id, "local"),
    requestDeadEnd("request", id, "local"),
    responseDeadEnd("response", id, "local") {

  // Tie off unused networks here.
  // This tile is only connected to request inputs and response outputs.
  iData(dataDeadEnd);
//  oData(dataDeadEnd);

  iCredit(creditDeadEnd);
//  oCredit(creditDeadEnd);

  iRequest(incomingRequests);
//  oRequest(requestDeadEnd);

  iResponse(responseDeadEnd);
  // Write to oResponse directly.
}

MemoryControllerTile::~MemoryControllerTile() {
  // Nothing
}

void MemoryControllerTile::end_of_elaboration() {
  // Register methods to deal with input/output.
  SC_METHOD(requestLoop);
  sensitive << incomingRequests.canReadEvent();
  dont_initialize();

  SC_METHOD(responseLoop);
  sensitive << iResponseFromMainMemory->canReadEvent();
  dont_initialize();
}

void MemoryControllerTile::requestLoop() {
  if (!oRequestToMainMemory->canWrite()) {
    next_trigger(oRequestToMainMemory->canWriteEvent());
  }
  else if (incomingRequests.canRead()) {
    Flit<Word> flit = incomingRequests.read();
    oRequestToMainMemory->write(flit);

    LOKI_LOG(3) << this->name() << " forwarding request to main memory: "
        << flit << endl;

    // TODO: allow multiple flits per cycle. #bandwidth
    if (incomingRequests.canRead())
      next_trigger(clock.posedge_event());
    // Else, default trigger: new request arrival
  }
}

void MemoryControllerTile::responseLoop() {
  if (!oResponse->canWrite()) {
    next_trigger(oResponse->canWriteEvent());
  }
  else if (iResponseFromMainMemory->canRead()) {
    Flit<Word> flit = iResponseFromMainMemory->read();
    oResponse->write(flit);

    LOKI_LOG(3) << this->name() << " forwarding response from main memory: "
        << flit << endl;

    // Wait for the clock edge rather than the next data arrival because there
    // may be other data lined up and ready to go immediately.
    // TODO: allow multiple flits per cycle. #bandwidth
    next_trigger(clock.posedge_event());
  }
  // else default trigger: new data to send
}
