/*
 * MissHandlingLogic.cpp
 *
 *  Created on: 8 Oct 2014
 *      Author: db434
 */

#include "MissHandlingLogic.h"
#include "../ComputeTile.h"
#include "../../Chip.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/Network.h"

MissHandlingLogic::MissHandlingLogic(const sc_module_name& name, ComponentID id) :
    LokiComponent(name, id),
    clock("clock"),
    oRequestToNetwork("oRequestToNetwork"),
    iResponseFromNetwork("iResponseFromNetwork"),
    oReadyForResponse("oReadyForResponse"),
    oResponseToBanks("oResponseToBanks"),
    oResponseTarget("oResponseTarget"),
    oResponseToNetwork("oResponseToNetwork"),
    iRequestFromNetwork("iRequestFromNetwork"),
    oReadyForRequest("oReadyForRequest"),
    oRequestToBanks("oRequestToBanks"),
    oRequestTarget("oRequestTarget"),
    iClaimRequest(MEMS_PER_TILE, "iClaimRequest"),
    oRequestClaimed("oRequestClaimed"),
    directory(DIRECTORY_SIZE),
    requestMux("request_mux", MEMS_PER_TILE),
    responseMux("response_mux", MEMS_PER_TILE) {

  TileID memController = nearestMemoryController();
  directory.initialise(memController);

  requestDestination = ChannelID();
  newLocalRequest = true;
  newRemoteRequest = true;
  requestHeaderValid = false;

  rngState = 0x3F;  // Same seed as Verilog uses (see nextTargetBank()).

  iRequestFromBanks.init(requestMux.iData, "iRequestFromBanks");
  iResponseFromBanks.init(responseMux.iData, "iResponseFromBanks");

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
  sensitive << iResponseFromNetwork;
  dont_initialize();

  SC_METHOD(remoteRequestLoop);
  sensitive << iRequestFromNetwork;
  dont_initialize();

  SC_METHOD(sendResponseLoop);
  sensitive << muxedResponse;
  dont_initialize();

  SC_METHOD(requestClaimLoop);
  for (uint i=0; i<iClaimRequest.size(); i++)
    sensitive << iClaimRequest[i];
  // do initialise
}

bool MissHandlingLogic::backedByMainMemory(MemoryAddr address) const {
  MemoryAddr newAddress = directory.updateAddress(address);
  TileID nextTile = directory.getNextTile(address);
  bool scratchpad = directory.inScratchpad(address);

  // This is the final translation if the target tile is in scratchpad mode.
  // Otherwise, pass the request on to the next level of cache.
  if (scratchpad)
    return false;
  else
    return chip()->backedByMainMemory(nextTile, newAddress);
}

MemoryAddr MissHandlingLogic::getAddressTranslation(MemoryAddr address) const {
  MemoryAddr newAddress = directory.updateAddress(address);
  TileID nextTile = directory.getNextTile(address);
  bool scratchpad = directory.inScratchpad(address);

  // This is the final translation if the target tile is in scratchpad mode.
  // Otherwise, pass the request on to the next level of cache.
  if (scratchpad)
    return newAddress;
  else
    return chip()->getAddressTranslation(nextTile, newAddress);
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
          // Adjust the flit based on its contents and the contents of the
          // directory.
          if (newLocalRequest) {
            flit = directory.updateRequest(flit);

            // Save the network destination so it can be reused for all other
            // flits in the same packet.
            requestDestination = flit.channelID();
          }
          else
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

  loki_assert(networkDataAvailable());
  NetworkResponse response = receiveFromNetwork();

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
    Instrumentation::Network::recordBandwidth(iRequestFromNetwork.name());
    LOKI_LOG << this->name() << " sending request to banks " << flit << endl;

    // For each new request, update the target bank. This bank services the
    // request in the event that no bank currently holds the required data.
    if (newRemoteRequest) {
      MemoryIndex targetBank;

      // Choose which bank should be responsible for the request.

      // In scratchpad mode, the bank is specified by the bits immediately
      // above the offset.
      if (flit.getMemoryMetadata().scratchpad)
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
  for (uint i=0; i<iClaimRequest.size(); i++) {
    if (iClaimRequest[i].read()) {
      loki_assert_with_message(!claimed, "Multiple L2 banks responding to request for 0x%x.", oRequestToBanks.read().payload().toUInt());
      claimed = true;
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
    Instrumentation::Network::recordBandwidth(oResponseToNetwork.name());

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


void MissHandlingLogic::sendOnNetwork(NetworkRequest request) {
  loki_assert(canSendOnNetwork());
  oRequestToNetwork.write(request);
  Instrumentation::Network::recordBandwidth(oRequestToNetwork.name());
}

bool MissHandlingLogic::canSendOnNetwork() const {
  return !oRequestToNetwork.valid();
}

const sc_event& MissHandlingLogic::canSendEvent() const {
  return oRequestToNetwork.ack_event();
}

NetworkResponse MissHandlingLogic::receiveFromNetwork() {
  loki_assert(iResponseFromNetwork.valid());
  Instrumentation::Network::recordBandwidth(iResponseFromNetwork.name());
  iResponseFromNetwork.ack();
  return iResponseFromNetwork.read();
}

bool MissHandlingLogic::networkDataAvailable() const {
  return iResponseFromNetwork.valid();
}

const sc_event& MissHandlingLogic::newNetworkDataEvent() const {
  return iResponseFromNetwork.default_event();
}

TileID MissHandlingLogic::nearestMemoryController() const {
  ComputeTile* tile = static_cast<ComputeTile*>(this->get_parent_object());
  return tile->chip()->nearestMemoryController(id.tile);
}

Chip* MissHandlingLogic::chip() const {
  return static_cast<ComputeTile*>(this->get_parent_object())->chip();
}
