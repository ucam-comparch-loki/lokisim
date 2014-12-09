/*
 * MissHandlingLogic.cpp
 *
 *  Created on: 8 Oct 2014
 *      Author: db434
 */

#include "MissHandlingLogic.h"

// Parameter to choose how main memory should be accessed.
//   true:  send all requests out over the on-chip network
//   false: magic connection from anywhere on the chip
#define MAIN_MEMORY_ON_NETWORK (false)

MissHandlingLogic::MissHandlingLogic(const sc_module_name& name, ComponentID id) :
    Component(name, id),
    inputMux("mux", MEMS_PER_TILE) {

  iRequestFromBanks.init(MEMS_PER_TILE);

  state = MHL_READY;
  flitsRemaining = 0;

  for (uint i=0; i<iRequestFromBanks.length(); i++)
    inputMux.iData[i](iRequestFromBanks[i]);
  inputMux.oData(muxOutput);
  inputMux.iHold(holdMux);

  // Ready signal for compatibility with network. It isn't required since we
  // only ever issue a request if there's space to receive a response.
  oReadyForResponse.initialize(true);
  holdMux.write(false);

  SC_METHOD(mainLoop);
}

void MissHandlingLogic::mainLoop() {
  switch (state) {
    case MHL_READY:
      if (!muxOutput.valid())
        next_trigger(muxOutput.default_event());
      else
        handleNewRequest();
      break;
    case MHL_FETCH:
      handleFetch();
      break;
    case MHL_STORE:
      handleStore();
      break;
  }
}

void MissHandlingLogic::handleNewRequest() {
  holdMux.write(true);

  MemoryRequest request = muxOutput.read();

  switch (request.getOperation()) {
    case MemoryRequest::FETCH_LINE:
      state = MHL_FETCH;
      flitsRemaining = request.getLineSize() / BYTES_PER_WORD;
      if (DEBUG)
        cout << this->name() << " requesting " << flitsRemaining << " words from 0x" << std::hex << request.getPayload() << std::dec << endl;
      break;
    case MemoryRequest::STORE_LINE:
      state = MHL_STORE;

      // Send the address plus the cache line.
      flitsRemaining = 1 + request.getLineSize() / BYTES_PER_WORD;

      if (DEBUG)
        cout << this->name() << " flushing " << flitsRemaining << " words to 0x" << std::hex << request.getPayload() << std::dec << endl;
      break;
    default:
      throw InvalidOptionException("miss handling logic new request", request.getOperation());
      break;
  }

  next_trigger(sc_core::SC_ZERO_TIME);
}

void MissHandlingLogic::handleFetch() {
  if (networkDataAvailable()) {
    // The memory bank should be able to consume all data immediately.
    assert(!oDataToBanks.valid());

    Word data = receiveFromNetwork();

    if (DEBUG)
      cout << this->name() << " received " << data << endl;

    flitsRemaining--;
    oDataToBanks.write(data);
    if (flitsRemaining == 0)
      handleEndOfRequest();
    else
      next_trigger(newNetworkDataEvent());
  }
  else if (!canSendOnNetwork()) {
    next_trigger(canSendEvent());
  }
  else {
    sendOnNetwork(muxOutput.read());
    muxOutput.ack();
    next_trigger(newNetworkDataEvent());
  }
}

void MissHandlingLogic::handleStore() {
  if (canSendOnNetwork()) {
    sendOnNetwork(muxOutput.read());
    muxOutput.ack();
    flitsRemaining--;

    if (flitsRemaining == 0)
      handleEndOfRequest();
    else
      next_trigger(muxOutput.default_event());
  }
  else {
    next_trigger(canSendEvent());
  }
}

void MissHandlingLogic::handleEndOfRequest() {
  state = MHL_READY;
  holdMux.write(false);
  next_trigger(clock.posedge_event());
}


// All the following methods behave differently depending on how main memory
// is accessed.


void MissHandlingLogic::sendOnNetwork(MemoryRequest request) {
  assert(canSendOnNetwork());

  if (MAIN_MEMORY_ON_NETWORK) {
    ChannelID destination = getDestination(request);
    NetworkRequest flit(request, destination);
    oRequestToNetwork.write(flit);
  }
  else {
    oRequestToBM.write(request);
  }
}

bool MissHandlingLogic::canSendOnNetwork() const {
  if (MAIN_MEMORY_ON_NETWORK)
    return !oRequestToNetwork.valid();
  else {
    return !oRequestToBM.valid();
  }
}

const sc_event& MissHandlingLogic::canSendEvent() const {
  if (MAIN_MEMORY_ON_NETWORK)
    return oRequestToNetwork.ack_event();
  else {
    return oRequestToBM.ack_event();
  }
}

Word MissHandlingLogic::receiveFromNetwork() {
  if (MAIN_MEMORY_ON_NETWORK) {
    assert(iResponseFromNetwork.valid());
    iResponseFromNetwork.ack();
    return iResponseFromNetwork.read().payload();
  }
  else {
    assert(iDataFromBM.valid());
    iDataFromBM.ack();
    return iDataFromBM.read();
  }
}

bool MissHandlingLogic::networkDataAvailable() const {
  if (MAIN_MEMORY_ON_NETWORK)
    return iResponseFromNetwork.valid();
  else {
    return iDataFromBM.valid();
  }
}

const sc_event& MissHandlingLogic::newNetworkDataEvent() const {
  if (MAIN_MEMORY_ON_NETWORK)
    return iResponseFromNetwork.default_event();
  else {
    return iDataFromBM.default_event();
  }
}

ChannelID MissHandlingLogic::getDestination(MemoryRequest request) {
  // For now, return the fixed address of the memory controller.
  // In the future, this is where we will look in the directory to determine
  // which L2 tile to access.
  return ChannelID(NUM_TILES,0,0);
}
