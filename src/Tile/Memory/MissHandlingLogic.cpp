/*
 * MissHandlingLogic.cpp
 *
 *  Created on: 8 Oct 2014
 *      Author: db434
 */

#include "MissHandlingLogic.h"
#include "../ComputeTile.h"
#include "../../Chip.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Utility/Assert.h"

MissHandlingLogic::MissHandlingLogic(const sc_module_name& name,
                                     const tile_parameters_t& params) :
    LokiComponent(name),
    clock("clock"),
    oRequestToNetwork("oRequestToNetwork"),
    iResponseFromNetwork("iResponseFromNetwork"),
    iRequestFromBanks("iRequestFromBanks"),
    oResponseToBanks("oResponseToBanks"),
    directory(params.directory.size),
    requestsFromBanks("requestsFromBanks", 1),
    responsesFromNetwork("responsesFromNetwork", 1) {

  TileID memController = nearestMemoryController();
  directory.initialise(memController);

  requestDestination = ChannelID();
  newLocalRequest = true;
  requestHeaderValid = false;

  iResponseFromNetwork(responsesFromNetwork);
  iRequestFromBanks(requestsFromBanks);
  oResponseToBanks(responsesFromNetwork);

  SC_METHOD(localRequestLoop);
  sensitive << requestsFromBanks.canReadEvent();
  dont_initialize();
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
    return chip().backedByMainMemory(nextTile, newAddress);
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
    return chip().getAddressTranslation(nextTile, newAddress);
}

void MissHandlingLogic::localRequestLoop() {

  loki_assert(requestsFromBanks.canRead());
  NetworkRequest flit = requestsFromBanks.read();
  bool endOfPacket = flit.getMetadata().endOfPacket;

  // Stall until it is possible to send on the network.
  if (!oRequestToNetwork->canWrite())
    next_trigger(oRequestToNetwork->canWriteEvent());
  else {

    // Continuation of an operation which takes place at the directory.
    if (requestHeaderValid) {
      switch (requestHeader.getMemoryMetadata().opcode) {
        case UPDATE_DIRECTORY_ENTRY:
          handleDirectoryUpdate(flit);
          break;

        case UPDATE_DIRECTORY_MASK:
          handleDirectoryMaskUpdate(flit);
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
      // Don't execute directory update commands here if they have skipped this
      // level of cache. Since the MHL doesn't know whether to check the Skip L1
      // or Skip L2 bit, we let the MemoryBank copy whichever bit is appropriate
      // into the Scratchpad bit. This bit is always overwritten when forwarding
      // the request.
      MemoryOpcode op = flit.getMemoryMetadata().opcode;
      bool updateDirectory = (op == UPDATE_DIRECTORY_ENTRY ||
                              op == UPDATE_DIRECTORY_MASK)
                          && !flit.getMemoryMetadata().scratchpad;

      if (updateDirectory) {
        // Store the head flit and wait for the payload.
        requestHeader = flit;
        requestHeaderValid = true;
      }
      else {
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

        oRequestToNetwork->write(flit);
      }
    }

    newLocalRequest = endOfPacket;

    // The mux may have other data ready to provide, so also wait until a
    // clock edge.
    // TODO Allow increased bandwidth.
    next_trigger(requestsFromBanks.canReadEvent() & clock.posedge_event());

  }
}

void MissHandlingLogic::handleDirectoryUpdate(const NetworkRequest& flit) {
  MemoryRequest request = static_cast<MemoryRequest>(flit.payload());

  MemoryAddr address = requestHeader.payload().toUInt();
  unsigned int entry = directory.getEntry(address);
  uint data = request.getPayload();

  directory.setEntry(entry, data);
}

void MissHandlingLogic::handleDirectoryMaskUpdate(const NetworkRequest& flit) {
  MemoryRequest request = static_cast<MemoryRequest>(flit.payload());

  unsigned int maskLSB = request.getPayload();

  directory.setBitmaskLSB(maskLSB);
}

TileID MissHandlingLogic::nearestMemoryController() const {
  return chip().nearestMemoryController(tile().id);
}

ComputeTile& MissHandlingLogic::tile() const {
  return *static_cast<ComputeTile*>(this->get_parent_object());
}

Chip& MissHandlingLogic::chip() const {
  return tile().chip();
}
