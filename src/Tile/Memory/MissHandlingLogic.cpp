/*
 * MissHandlingLogic.cpp
 *
 *  Created on: 8 Oct 2014
 *      Author: db434
 */

#include "../../Tile/Memory/MissHandlingLogic.h"

#include "../../Utility/Assert.h"

// Parameter to choose how main memory should be accessed.
//   true:  send all requests out over the on-chip network
//   false: magic connection from anywhere on the chip
#define MAIN_MEMORY_ON_NETWORK (false)

MissHandlingLogic::MissHandlingLogic(const sc_module_name& name, ComponentID id) :
    LokiComponent(name, id),
    directory(DIRECTORY_SIZE),
    requestMux("request_mux", MEMS_PER_TILE),
    responseMux("response_mux", MEMS_PER_TILE) {

  // Start off assuming this is the home tile for all data. This means that if
  // we experience a cache miss, we will go straight to main memory to
  // retrieve it.
  TileID tile = id.tile;//*/TileID(2,1);
  directory.initialise(tile);

  requestDestination = ChannelID();
  newLocalRequest = true;
  newRemoteRequest = true;
  requestHeaderValid = false;

  rngState = 0x3F;  // Same seed as Verilog uses (see nextTargetBank()).

  iRequestFromBanks.init(requestMux.iData);
  iResponseFromBanks.init(responseMux.iData);
  iClaimRequest.init(MEMS_PER_TILE);

  requestMux.iData(iRequestFromBanks);
  requestMux.oData(muxedRequest);
  requestMux.iHold(holdRequestMux);

  responseMux.iData(iResponseFromBanks);
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
  sensitive << muxedRequest;
  dont_initialize();

  SC_METHOD(receiveResponseLoop);
  sensitive << iResponseFromNetwork << iResponseFromBM;
  dont_initialize();

  SC_METHOD(remoteRequestLoop);
  sensitive << iRequestFromNetwork;
  dont_initialize();

  SC_METHOD(sendResponseLoop);
  sensitive << muxedResponse;
  dont_initialize();

  SC_METHOD(requestClaimLoop);
  for (uint i=0; i<iClaimRequest.length(); i++)
    sensitive << iClaimRequest[i];
  // do initialise
}

void MissHandlingLogic::localRequestLoop() {

  loki_assert(muxedRequest.valid());
  NetworkRequest flit = muxedRequest.read();
  bool endOfPacket = flit.getMetadata().endOfPacket;
  holdRequestMux.write(!endOfPacket);

  // Stall until it is possible to send on the network.
  if (!canSendOnNetwork())
    next_trigger(canSendEvent());
  else {

    // Continuation of an operation which takes place at the directory.
    if (requestHeaderValid) {
      switch (requestHeader.getMemoryMetadata().opcode) {
        case UPDATE_DIRECTORY_ENTRY:
          handleDirectoryUpdate();
          break;

        case UPDATE_DIRECTORY_MASK:
          handleDirectoryMaskUpdate();
          break;

        default:
          loki_assert_with_message(false,
              "Request type = %s", memoryOpName(requestHeader.getMemoryMetadata().opcode).c_str());
          break;
      }

      requestHeaderValid = false;
    }
    // All header flits, and flits which are being forwarded to another tile.
    else {
      switch (flit.getMemoryMetadata().opcode) {
        // If the operation will take place at the directory, store the header
        // until the rest of the information arrives.
        case UPDATE_DIRECTORY_ENTRY:
        case UPDATE_DIRECTORY_MASK:
          requestHeader = flit;
          requestHeaderValid = true;
          break;

        default: {
          // Use the memory address in the first flit of each packet to determine
          // which network address the whole packet should be forwarded to.
          if (newLocalRequest) {
            MemoryAddr address = flit.payload().toUInt();
            requestDestination = getDestination(address);
            flit.setPayload(directory.updateAddress(address));
          }

          flit.setChannelID(requestDestination);

          LOKI_LOG << this->name() << " sending request " << flit << endl;

          sendOnNetwork(flit);

          break;
        }
      }
    }

    muxedRequest.ack();
    newLocalRequest = endOfPacket;

    // The mux may have other data ready to provide, so also wait until a
    // clock edge.
    next_trigger(muxedRequest.default_event() & clock.posedge_event());

  }
}

void MissHandlingLogic::handleDirectoryUpdate() {
  MemoryRequest request = static_cast<MemoryRequest>(muxedRequest.read().payload());

  MemoryAddr address = requestHeader.payload().toUInt();
  unsigned int entry = directory.getEntry(address);
  uint data = request.getPayload();

  directory.setEntry(entry, data);
}

void MissHandlingLogic::handleDirectoryMaskUpdate() {
  MemoryRequest request = static_cast<MemoryRequest>(muxedRequest.read().payload());

  unsigned int maskLSB = request.getPayload();

  directory.setBitmaskLSB(maskLSB);
}

void MissHandlingLogic::receiveResponseLoop() {
  if (oResponseToBanks.valid()) {
    next_trigger(oResponseToBanks.ack_event());
    return;
  }

  NetworkResponse response;
  if (iResponseFromBM.valid()) {
    response = iResponseFromBM.read();
    iResponseFromBM.ack();
  }
  else {
    loki_assert(iResponseFromNetwork.valid());
    response = iResponseFromNetwork.read();
    iResponseFromNetwork.ack();
  }

  LOKI_LOG << this->name() << " received " << response << endl;

  oResponseToBanks.write(response);
  oResponseTarget.write(response.channelID().component.position);
}


void MissHandlingLogic::remoteRequestLoop() {
  // Stall until the memory banks are capable of receiving a new flit.
  if (oRequestToBanks.valid()) {
    next_trigger(oRequestToBanks.ack_event());
  }
  else {
    loki_assert(iRequestFromNetwork.valid());

    NetworkRequest flit = iRequestFromNetwork.read();
    LOKI_LOG << this->name() << " sending request to banks " << flit << endl;

    // For each new request, update the target bank. This bank services the
    // request in the event that no bank currently holds the required data.
    if (newRemoteRequest) {
      MemoryIndex targetBank;

      // Choose which bank should be responsible for the request.

      // In scratchpad mode, the bank is specified by the bits immediately
      // above the offset.
      if (flit.getMemoryMetadata().scratchpadL2)
        targetBank = (flit.payload().toUInt() >> 5) & (MEMS_PER_TILE - 1);
      // Push line operations specify the target bank with the lowest bits.
      else if (flit.getMemoryMetadata().opcode == PUSH_LINE)
        targetBank = flit.payload().toUInt() & (MEMS_PER_TILE - 1);
      // Otherwise, the target bank is random.
      else
        targetBank = nextTargetBank();

      oRequestTarget.write(targetBank);
    }

    oRequestToBanks.write(flit);
    iRequestFromNetwork.ack();

    newRemoteRequest = flit.getMetadata().endOfPacket;
  }
}

void MissHandlingLogic::requestClaimLoop() {
  bool claimed = false;
  for (uint i=0; i<iClaimRequest.length(); i++) {
    if (iClaimRequest[i].read()) {
      claimed = true;
      break;
    }
  }
  oRequestClaimed.write(claimed);
}

void MissHandlingLogic::sendResponseLoop() {
  if (oResponseToNetwork.valid()) {
    next_trigger(oResponseToNetwork.ack_event() & clock.posedge_event());
  }
  else if (muxedResponse.valid()) {
    oResponseToNetwork.write(muxedResponse.read());

    // Optional: if there is more data to come in this packet, wait until it
    // has all been sent before switching to another packet.
    holdResponseMux.write(!muxedResponse.read().getMetadata().endOfPacket);

    muxedResponse.ack();

    // Wait for the clock edge rather than the next data arrival because the
    // mux may have other data lined up and ready to go immediately.
    next_trigger(clock.posedge_event());
  }
}

MemoryIndex MissHandlingLogic::nextTargetBank() {
  // Based on the Verilog: cache/l2_prng.sv.

  MemoryIndex targetBank = oRequestTarget.read();

  // Just using the state on its own to generate the target leads to
  // uneven bank choice (note the period is not a multiple of 8!). The
  // following technique has been confirmed experimentally to lead to a
  // period of num_banks * 63 for 1 <= num_banks <= 32.
  //
  // Admittedly this verilog breaks for num_banks <= 2, but you get the
  // idea.
  //
  // Rotate right 1.
  targetBank = (targetBank - 1) % MEMS_PER_TILE;
  if (rngState & 0x1) {
    // Rotate right 1.
    targetBank = (targetBank - 1) % MEMS_PER_TILE;
  }
  if (rngState & 0x4) {
    // Rotate right 4.
    targetBank = (targetBank - 4) % MEMS_PER_TILE;
  }

  // Linear feedback shift register RNG. The generator polynomial used
  // here is x^6 + x^5 + 1. Has a period of 63. Do not change this
  // without evaluating the impact on the code above! PRNGs are hard and
  // this one has been experimentally tuned to yield a good period.
  rngState = (rngState >> 1) ^ ((rngState & 0x1) ? 0x30 : 0x0);

  return targetBank;
}


// All the following methods behave differently depending on how main memory
// is accessed.


void MissHandlingLogic::sendOnNetwork(NetworkRequest request) {
  loki_assert(canSendOnNetwork());

  if ((requestDestination != memoryControllerAddress()) || MAIN_MEMORY_ON_NETWORK)
    oRequestToNetwork.write(request);
  else
    oRequestToBM.write(request);
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
    loki_assert(iResponseFromNetwork.valid());
    iResponseFromNetwork.ack();
    return iResponseFromNetwork.read();
  }
  else {
    loki_assert(iResponseFromBM.valid());
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
