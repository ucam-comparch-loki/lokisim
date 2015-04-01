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
  TileIndex tile = id.tile.flatten();//*/TileID(2,1).flatten();
  directory.initialise(tile);

  localState = MHL_READY;
  remoteState = MHL_READY;
  requestDestination = ChannelID();
  responseDestination = ChannelID();
  requestFlitsRemaining = 0;
  responseFlitsRemaining = 0;

  iRequestFromBanks.init(requestMux.iData);
  iDataFromBanks.init(responseMux.iData);

  requestMux.iData(iRequestFromBanks);
  requestMux.oData(muxedRequest);
  requestMux.iHold(holdRequestMux);

  responseMux.iData(iDataFromBanks);
  responseMux.oData(muxedResponse);
  responseMux.iHold(holdResponseMux);

  // Ready signal for compatibility with network. It isn't required since we
  // only ever issue a request if there's space to receive a response.
  oReadyForResponse.initialize(true);
  oReadyForRequest.initialize(true);
  oRequestTarget.initialize(0);
  oResponseTarget.initialize(0);
  holdRequestMux.write(false);
  holdResponseMux.write(false);

  SC_METHOD(localRequestLoop);
  SC_METHOD(remoteRequestLoop);

  SC_METHOD(receiveResponseLoop);
  sensitive << iResponseFromNetwork << iResponseFromBM;
  dont_initialize();
}

void MissHandlingLogic::localRequestLoop() {
  switch (localState) {
    case MHL_READY:
      if (!muxedRequest.valid())
        next_trigger(muxedRequest.default_event() & clock.posedge_event());
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
      requestFlitsRemaining = request.getLineSize();
      MemoryAddr address = request.getPayload();
      requestDestination = getDestination(address);

      if (DEBUG)
        cout << this->name() << " requesting " << request.getLineSize() << " words from 0x" << std::hex << address << std::dec << " on tile " << requestDestination.component.tile << endl;
      break;
    }

    case MemoryRequest::STORE_LINE: {
      localState = MHL_STORE;

      // Send the address plus the cache line.
      requestFlitsRemaining = 1 + request.getLineSize();
      MemoryAddr address = request.getPayload();
      requestDestination = getDestination(address);

      if (DEBUG)
        cout << this->name() << " flushing " << request.getLineSize() << " words to 0x" << std::hex << address << std::dec << " on tile " << requestDestination.component.tile << endl;
      break;
    }

    case MemoryRequest::DIRECTORY_UPDATE:
      handleDirectoryUpdate();
      break;

    case MemoryRequest::DIRECTORY_MASK_UPDATE:
      handleDirectoryMaskUpdate();
      break;

    default:
      throw InvalidOptionException("miss handling logic local request operation", request.getOperation());
      break;

  } // end switch

  next_trigger(sc_core::SC_ZERO_TIME);
}

void MissHandlingLogic::handleLocalFetch() {
  if (!canSendOnNetwork()) {
    next_trigger(canSendEvent() & clock.posedge_event());
  }
  else {
    sendOnNetwork(muxedRequest.read(), true);
    muxedRequest.ack();
    next_trigger(newNetworkDataEvent() & clock.posedge_event());

    // Move on to next request while we wait for data to come back.
    endLocalRequest();
  }
}

void MissHandlingLogic::handleLocalStore() {
  if (canSendOnNetwork()) {
    requestFlitsRemaining--;

    if (requestFlitsRemaining == 0) {
      sendOnNetwork(muxedRequest.read(), true);
      endLocalRequest();
    }
    else {
      sendOnNetwork(muxedRequest.read(), false);
      next_trigger(muxedRequest.default_event() & clock.posedge_event());
    }

    muxedRequest.ack();
  }
  else {
    next_trigger(canSendEvent() & clock.posedge_event());
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

void MissHandlingLogic::receiveResponseLoop() {
  assert(!oDataToBanks.valid());

  NetworkResponse response;
  if (iResponseFromBM.valid()) {
    response = iResponseFromBM.read();
    iResponseFromBM.ack();
  }
  else {
    assert(iResponseFromNetwork.valid());
    response = iResponseFromNetwork.read();
    iResponseFromNetwork.ack();
  }

  if (DEBUG)
    cout << this->name() << " received " << response << endl;

  oDataToBanks.write(response.payload());
  oResponseTarget.write(response.channelID().component.position);
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
  NetworkRequest flit = iRequestFromNetwork.read();
  MemoryRequest request = flit.payload();

  // Update the target bank - the one to service the request in the event that
  // no bank currently holds the required data.
  // TODO: use a more realistic random number generator.
  MemoryIndex targetBank = rand() % MEMS_PER_TILE;
  oRequestTarget.write(targetBank);

  switch (request.getOperation()) {

    case MemoryRequest::FETCH_LINE: {
      remoteState = MHL_FETCH;

      responseFlitsRemaining = request.getLineSize();
      MemoryAddr address = request.getPayload();
      TileID sourceTile(request.getSourceTile());
      MemoryIndex sourceBank = request.getSourceBank();
      responseDestination = ChannelID(sourceTile.x, sourceTile.y, sourceBank, 0);

      if (DEBUG)
        cout << this->name() << " requesting " << request.getLineSize() << " words from 0x" << std::hex << address << std::dec << " (proposed bank " << targetBank << ")" << endl;
      break;
    }

    case MemoryRequest::STORE_LINE: {
      remoteState = MHL_STORE;

      // Send the address plus the cache line.
      responseFlitsRemaining = request.getLineSize() + 1; // +1 for this header flit
      MemoryAddr address = request.getPayload();

      if (DEBUG)
        cout << this->name() << " flushing " << request.getLineSize() << " words to 0x" << std::hex << address << std::dec << " (proposed bank " << targetBank << ")" << endl;
      break;
    }

    default:
      throw InvalidOptionException("miss handling logic remote request operation", request.getOperation());
      break;

  } // end switch

  // Move to the next state ASAP.
  next_trigger(sc_core::SC_ZERO_TIME);
}

void MissHandlingLogic::handleRemoteFetch() {
  // Receiving data from memory banks.
  if (muxedResponse.valid()) {
    // No buffering - wait until the output port is available.
    if (oResponseToNetwork.valid())
      next_trigger(oResponseToNetwork.ack_event() & clock.posedge_event());
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
        next_trigger(muxedResponse.default_event() & clock.posedge_event());
      }

      oResponseToNetwork.write(response);
    }
  }
  // No data from memory banks => need to issue the fetch to them.
  else {
    oRequestToBanks.write(iRequestFromNetwork.read().payload());
    iRequestFromNetwork.ack();
    next_trigger(muxedResponse.default_event() & clock.posedge_event());
  }
}

void MissHandlingLogic::handleRemoteStore() {
  // The banks may still be waiting to serve the previous request if there was
  // a cache miss.
  MemoryRequest request = iRequestFromNetwork.read().payload();

  oRequestToBanks.write(request);
  iRequestFromNetwork.ack();
  responseFlitsRemaining--;

  if (responseFlitsRemaining == 0) {
    assert(iRequestFromNetwork.read().endOfPacket());
    endRemoteRequest();
  }
  else
    next_trigger(iRequestFromNetwork.default_event() & oRequestToBanks.ack_event());
}

void MissHandlingLogic::endRemoteRequest() {
  remoteState = MHL_READY;
  holdResponseMux.write(false);
  next_trigger(clock.posedge_event());
}


// All the following methods behave differently depending on how main memory
// is accessed.


void MissHandlingLogic::sendOnNetwork(MemoryRequest request, bool endOfPacket) {
  assert(canSendOnNetwork());

  if ((requestDestination != memoryControllerAddress()) || MAIN_MEMORY_ON_NETWORK) {
    NetworkRequest flit(request, requestDestination);
    flit.setEndOfPacket(endOfPacket);
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

NetworkResponse MissHandlingLogic::receiveFromNetwork() {
  if ((requestDestination != memoryControllerAddress()) || MAIN_MEMORY_ON_NETWORK) {
    assert(iResponseFromNetwork.valid());
    iResponseFromNetwork.ack();
    return iResponseFromNetwork.read();
  }
  else {
    assert(iResponseFromBM.valid());
    iResponseFromBM.ack();
    return iResponseFromBM.read();
  }
}

bool MissHandlingLogic::networkDataAvailable() const {
  if ((requestDestination != memoryControllerAddress()) || MAIN_MEMORY_ON_NETWORK)
    return iResponseFromNetwork.valid();
  else {
    return iResponseFromBM.valid();
  }
}

const sc_event& MissHandlingLogic::newNetworkDataEvent() const {
  if ((requestDestination != memoryControllerAddress()) || MAIN_MEMORY_ON_NETWORK)
    return iResponseFromNetwork.default_event();
  else {
    return iResponseFromBM.default_event();
  }
}

ChannelID MissHandlingLogic::getDestination(MemoryAddr address) const {
  TileID tile = directory.getTile(address);

  // The data should be on this tile, but isn't - go to main memory.
  if (tile == id.tile)
    return memoryControllerAddress();
  // The data should be on the tile indicated by the directory.
  else
    return ChannelID(tile.x, tile.y, 0, 0);
}

ChannelID MissHandlingLogic::memoryControllerAddress() const {
  return ChannelID(2, 0, 0, 0);
}
