/*
 * MissHandlingLogic.cpp
 *
 *  Created on: 8 Oct 2014
 *      Author: db434
 */

#include "MissHandlingLogic.h"

MissHandlingLogic::MissHandlingLogic(const sc_module_name& name, ComponentID id) :
    Component(name, id),
    inputMux("mux", MEMS_PER_TILE) {

  iRequestFromBanks.init(MEMS_PER_TILE);

  state = MHL_READY;

  for (uint i=0; i<iRequestFromBanks.length(); i++)
    inputMux.iData[i](iRequestFromBanks[i]);
  inputMux.oData(muxOutput);
  inputMux.iHold(holdMux);

  holdMux.write(false);

  SC_METHOD(mainLoop);
}

void MissHandlingLogic::mainLoop() {
  switch (state) {
    case MHL_READY:
      handleNewRequest();
      // No break; handleNewRequest chooses the next state and we can start
      // immediately.
    case MHL_FETCH:
      handleFetch();
      break;
    case MHL_WB:
      handleWriteBack();
      break;
    case MHL_ALLOC:
      handleAllocate();
      break;
    case MHL_ALLOCHIT:
      handleAllocateHit();
      break;
  } // no break
}

void MissHandlingLogic::handleNewRequest() {
  if (!muxOutput.valid()) {
    next_trigger(muxOutput.default_event());
    return;
  }

  holdMux.write(true);

  MemoryRequest request = muxOutput.read();
  switch (request.getOperation()) {
    case MemoryRequest::IPK_READ:
      state = MHL_FETCH;
      break;
    case MemoryRequest::STORE_LINE:
      state = MHL_WB;
      break;
    case MemoryRequest::FETCH_LINE:
      state = MHL_ALLOC;
      break;
    default:
      throw InvalidOptionException("miss handling logic new request", request.getOperation());
      break;
  }
}

void MissHandlingLogic::handleFetch() {
  if (networkDataAvailable()) {
    // The memory bank should be able to consume all data immediately.
    assert(!oDataToBanks.valid());

    NetworkResponse data = receiveFromNetwork();
    oDataToBanks.write(data.payload());
    if (data.endOfPacket())
      handleEndOfRequest();
  }
  else if (!canSendOnNetwork()) {
    next_trigger(canSendEvent());
  }
  else {
    sendOnNetwork(muxOutput.read().payload());
    muxOutput.ack();
    next_trigger(newNetworkDataEvent());
  }
}

void MissHandlingLogic::handleWriteBack() {
  if (canSendOnNetwork()) {
    sendOnNetwork(muxOutput.read().payload());
    muxOutput.ack();

    if (muxOutput.read().endOfPacket())
      handleEndOfRequest();
  }
  else {
    next_trigger(canSendEvent());
  }
}

void MissHandlingLogic::handleAllocate() {

}

void MissHandlingLogic::handleAllocateHit() {

}

void MissHandlingLogic::handleEndOfRequest() {
  state = MHL_READY;
  holdMux.write(false);
  next_trigger(clock.posedge_event());
}

void MissHandlingLogic::sendOnNetwork(MemoryRequest request) {
  assert(canSendOnNetwork());
  ChannelID destination = getDestination(request);
  NetworkRequest flit(request, destination);
  oRequestToNetwork.write(flit);
}

bool MissHandlingLogic::canSendOnNetwork() const {
  return !oRequestToNetwork.valid();
}

sc_event MissHandlingLogic::canSendEvent() const {
  return oRequestToNetwork.ack_event();
}

NetworkResponse MissHandlingLogic::receiveFromNetwork() {
  assert(iResponseFromNetwork.valid());
  iResponseFromNetwork.ack();
  return iResponseFromNetwork.read();
}

bool MissHandlingLogic::networkDataAvailable() const {
  return iResponseFromNetwork.valid();
}

sc_event MissHandlingLogic::newNetworkDataEvent() const {
  return iResponseFromNetwork.default_event();
}

ChannelID MissHandlingLogic::getDestination(MemoryRequest request) {
  // For now, return the fixed address of the memory controller.
  // In the future, this is where we will look in the directory to determine
  // which L2 tile to access.
  return ChannelID(2,0,0);
}
