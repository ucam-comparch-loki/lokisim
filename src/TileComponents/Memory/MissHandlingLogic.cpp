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
    directory(DIRECTORY_SIZE),
    requestMux("request_mux", MEMS_PER_TILE),
    responseMux("response_mux", MEMS_PER_TILE) {

  // Start off assuming this is the home tile for all data. This means that if
  // we experience a cache miss, we will go straight to main memory to
  // retrieve it.
  directory.initialise(id.getTile());

  iRequestFromBanks.init(MEMS_PER_TILE);
  iDataFromBanks.init(MEMS_PER_TILE);

  localState = MHL_READY;
  remoteState = MHL_READY;
  requestDestination = ChannelID();
  responseDestination = ChannelID();
  requestFlitsRemaining = 0;
  responseFlitsRemaining = 0;

  for (uint i=0; i<iRequestFromBanks.length(); i++)
    requestMux.iData[i](iRequestFromBanks[i]);
  requestMux.oData(muxedRequest);
  requestMux.iHold(holdRequestMux);

  for (uint i=0; i<iDataFromBanks.length(); i++)
    responseMux.iData[i](iDataFromBanks[i]);
  responseMux.oData(muxedResponse);
  responseMux.iHold(holdResponseMux);

  // Ready signal for compatibility with network. It isn't required since we
  // only ever issue a request if there's space to receive a response.
  oReadyForResponse.initialize(true);
  oReadyForRequest.initialize(true);
  holdRequestMux.write(false);
  holdResponseMux.write(false);

  SC_METHOD(localRequestLoop);
  SC_METHOD(remoteRequestLoop);
}

void MissHandlingLogic::localRequestLoop() {
  switch (localState) {
    case MHL_READY:
      if (!muxedRequest.valid())
        next_trigger(muxedRequest.default_event());
      else
        handleNewLocalRequest();
      break;
    case MHL_FETCH:
      handleLocalFetch();
      break;
    case MHL_STORE:
      handleLocalStore();
      break;
  }
}

void MissHandlingLogic::handleNewLocalRequest() {
  holdRequestMux.write(true);

  MemoryRequest request = muxedRequest.read();

  switch (request.getOperation()) {

    case MemoryRequest::FETCH_LINE: {
      localState = MHL_FETCH;
      requestFlitsRemaining = request.getLineSize() / BYTES_PER_WORD;
      MemoryAddr address = request.getPayload();
      requestDestination = getDestination(address);

      if (DEBUG)
        cout << this->name() << " requesting " << requestFlitsRemaining << " words from 0x" << std::hex << address << std::dec << endl;
      break;
    }

    case MemoryRequest::STORE_LINE: {
      localState = MHL_STORE;

      // Send the address plus the cache line.
      requestFlitsRemaining = 1 + request.getLineSize() / BYTES_PER_WORD;
      MemoryAddr address = request.getPayload();
      requestDestination = getDestination(address);

      if (DEBUG)
        cout << this->name() << " flushing " << requestFlitsRemaining << " words to 0x" << std::hex << address << std::dec << endl;
      break;
    }

    case MemoryRequest::DIRECTORY_UPDATE:
      handleDirectoryUpdate();
      break;

    case MemoryRequest::DIRECTORY_MASK_UPDATE:
      handleDirectoryMaskUpdate();
      break;

    default:
      throw InvalidOptionException("miss handling logic new local request", request.getOperation());
      break;

  } // end switch

  next_trigger(sc_core::SC_ZERO_TIME);
}

void MissHandlingLogic::handleLocalFetch() {
  if (networkDataAvailable()) {
    // The memory bank should be able to consume all data immediately.
    assert(!oDataToBanks.valid());

    Word data = receiveFromNetwork();

    if (DEBUG)
      cout << this->name() << " received " << data << endl;

    requestFlitsRemaining--;
    oDataToBanks.write(data);
    if (requestFlitsRemaining == 0)
      endLocalRequest();
    else
      next_trigger(newNetworkDataEvent());
  }
  else if (!canSendOnNetwork()) {
    next_trigger(canSendEvent());
  }
  else {
    sendOnNetwork(muxedRequest.read());
    muxedRequest.ack();
    next_trigger(newNetworkDataEvent());
  }
}

void MissHandlingLogic::handleLocalStore() {
  if (canSendOnNetwork()) {
    sendOnNetwork(muxedRequest.read());
    muxedRequest.ack();
    requestFlitsRemaining--;

    if (requestFlitsRemaining == 0)
      endLocalRequest();
    else
      next_trigger(muxedRequest.default_event());
  }
  else {
    next_trigger(canSendEvent());
  }
}

void MissHandlingLogic::handleDirectoryUpdate() {
  MemoryRequest request = muxedRequest.read();
  muxedRequest.ack();

  unsigned int entry = request.getDirectoryEntry();
  TileIndex tile = request.getTile();

  directory.setEntry(entry, tile);

  endLocalRequest();
}

void MissHandlingLogic::handleDirectoryMaskUpdate() {
  MemoryRequest request = muxedRequest.read();
  muxedRequest.ack();

  unsigned int maskLSB = request.getPayload();

  directory.setBitmaskLSB(maskLSB);

  endLocalRequest();
}

void MissHandlingLogic::endLocalRequest() {
  localState = MHL_READY;
  holdRequestMux.write(false);
  next_trigger(clock.posedge_event());
}


void MissHandlingLogic::remoteRequestLoop() {
  switch (remoteState) {
    case MHL_READY:
      if (iRequestFromNetwork.valid())
        handleNewRemoteRequest();
      else
        next_trigger(iRequestFromNetwork.default_event());
      break;
    case MHL_FETCH:
      handleRemoteFetch();
      break;
    case MHL_STORE:
      handleRemoteStore();
      break;
  }
}

void MissHandlingLogic::handleNewRemoteRequest() {
  MemoryRequest request = iRequestFromNetwork.read().payload();

  switch (request.getOperation()) {

    case MemoryRequest::FETCH_LINE: {
      remoteState = MHL_FETCH;

      responseFlitsRemaining = request.getLineSize() / BYTES_PER_WORD;
      MemoryAddr address = request.getPayload();
      responseDestination = ChannelID(request.getSourceTile(), 0, 0);

      if (DEBUG)
        cout << this->name() << " requesting " << responseFlitsRemaining << " words from 0x" << std::hex << address << std::dec << endl;
      break;
    }

    case MemoryRequest::STORE_LINE: {
      remoteState = MHL_STORE;

      // Send the address plus the cache line.
      responseFlitsRemaining = request.getLineSize() / BYTES_PER_WORD;
      MemoryAddr address = request.getPayload();

      if (DEBUG)
        cout << this->name() << " flushing " << responseFlitsRemaining << " words to 0x" << std::hex << address << std::dec << endl;
      break;
    }

    default:
      throw InvalidOptionException("miss handling logic new remote request", request.getOperation());
      break;

  } // end switch

  next_trigger(sc_core::SC_ZERO_TIME);
}

void MissHandlingLogic::handleRemoteFetch() {
  // Receiving data from memory banks.
  if (muxedResponse.valid()) {
    // No buffering - wait until the output port is available.
    if (oResponseToNetwork.valid())
      next_trigger(oResponseToNetwork.ack_event());
    // Output port is available - send flit.
    else {
      holdResponseMux.write(true);

      NetworkResponse response(muxedResponse.read(), responseDestination);
      muxedResponse.ack();

      responseFlitsRemaining--;
      if (responseFlitsRemaining == 0) {
        response.setEndOfPacket(true);
        endRemoteRequest();
      }
      else {
        response.setEndOfPacket(false);
        next_trigger(muxedResponse.default_event());
      }

      oResponseToNetwork.write(response);
    }
  }
  // No data from memory banks => need to issue the fetch to them.
  else {
    oRequestToBanks.write(iRequestFromNetwork.read().payload());
    iRequestFromNetwork.ack();
    next_trigger(muxedResponse.default_event());
  }
}

void MissHandlingLogic::handleRemoteStore() {
  assert(!oRequestToBanks.valid());

  oRequestToBanks.write(iRequestFromNetwork.read().payload());
  iRequestFromNetwork.ack();
  responseFlitsRemaining--;

  if (responseFlitsRemaining == 0)
    endRemoteRequest();
  else
    next_trigger(iRequestFromNetwork.default_event());
}

void MissHandlingLogic::endRemoteRequest() {
  remoteState = MHL_READY;
  holdResponseMux.write(false);
  next_trigger(clock.posedge_event());
}


// All the following methods behave differently depending on how main memory
// is accessed.


void MissHandlingLogic::sendOnNetwork(MemoryRequest request) {
  assert(canSendOnNetwork());

  if ((requestDestination != memoryControllerAddress()) || MAIN_MEMORY_ON_NETWORK) {

    // If this is the header flit, also include our tile ID so the remote memory
    // knows where to send data back to.
    // The tile ID could be added to all flits, but it's unnecessary simulation
    // overhead.
    MemoryRequest newRequest;
    if (request.getOperation() != MemoryRequest::PAYLOAD_ONLY)
      newRequest = MemoryRequest(request.getOperation(), request.getPayload(),
                                 request.getLineSize(),  id.getTile());
    else
      newRequest = request;

    NetworkRequest flit(newRequest, requestDestination);
    oRequestToNetwork.write(flit);
  }
  else {
    oRequestToBM.write(request);
  }
}

bool MissHandlingLogic::canSendOnNetwork() const {
  if ((requestDestination != memoryControllerAddress()) || MAIN_MEMORY_ON_NETWORK)
    return !oRequestToNetwork.valid();
  else {
    return !oRequestToBM.valid();
  }
}

const sc_event& MissHandlingLogic::canSendEvent() const {
  if ((requestDestination != memoryControllerAddress()) || MAIN_MEMORY_ON_NETWORK)
    return oRequestToNetwork.ack_event();
  else {
    return oRequestToBM.ack_event();
  }
}

Word MissHandlingLogic::receiveFromNetwork() {
  if ((requestDestination != memoryControllerAddress()) || MAIN_MEMORY_ON_NETWORK) {
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
  if ((requestDestination != memoryControllerAddress()) || MAIN_MEMORY_ON_NETWORK)
    return iResponseFromNetwork.valid();
  else {
    return iDataFromBM.valid();
  }
}

const sc_event& MissHandlingLogic::newNetworkDataEvent() const {
  if ((requestDestination != memoryControllerAddress()) || MAIN_MEMORY_ON_NETWORK)
    return iResponseFromNetwork.default_event();
  else {
    return iDataFromBM.default_event();
  }
}

ChannelID MissHandlingLogic::getDestination(MemoryAddr address) const {
  TileIndex tile = directory.getTile(address);

  // The data should be on this tile, but isn't - go to main memory.
  if (tile == id.getTile())
    return memoryControllerAddress();
  // The data should be on the tile indicated by the directory.
  else
    return ChannelID(tile,0,0);
}

ChannelID MissHandlingLogic::memoryControllerAddress() const {
  return ChannelID(NUM_TILES, 0, 0);
}
