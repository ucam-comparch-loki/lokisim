/*
 * L2Logic.cpp
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#include "L2Logic.h"

L2Logic::L2Logic(const sc_module_name& name, const tile_parameters_t& params) :
    LokiComponent(name),
    iRequestFromNetwork("iRequestFromNetwork"),
    oResponseToNetwork("oResponseToNetwork"),
    oRequestToBanks("oRequestToBanks"),
    oRequestTarget("oRequestTarget"),
    iClaimRequest("iClaimRequest", params.numMemories),
    iDelayRequest("iDelayRequest", params.numMemories),
    oRequestClaimed("oRequestClaimed"),
    oRequestDelayed("oRequestDelayed"),
    log2CacheLineSize(params.memory.log2CacheLineSize()),
    numMemoryBanks(params.numMemories),
    requestsFromNetwork("requestsFromNetwork", 1),
    responseMux("response_mux", params.numMemories) {

  rngState = 0x3F;  // Same seed as Verilog uses (see nextTargetBank()).
  newRemoteRequest = true;

  iRequestFromNetwork(requestsFromNetwork);

  iResponseFromBanks.init("iResponseFromBanks", responseMux.iData);

  responseMux.iData(iResponseFromBanks);
  responseMux.oData(muxedResponse);
  responseMux.iHold(holdResponseMux);

  // Ready signal for compatibility with network. It isn't required since we
  // only ever issue a request if there's space to receive a response.
  oRequestTarget.initialize(0);
  holdResponseMux.write(false);

  SC_METHOD(remoteRequestLoop);
  sensitive << requestsFromNetwork.canReadEvent();
  dont_initialize();

  SC_METHOD(sendResponseLoop);
  sensitive << muxedResponse;
  dont_initialize();

  SC_METHOD(requestClaimLoop);
  for (uint i=0; i<iClaimRequest.size(); i++)
    sensitive << iClaimRequest[i];
  // do initialise

  SC_METHOD(requestDelayLoop);
  for (uint i=0; i<iDelayRequest.size(); i++)
    sensitive << iDelayRequest[i];
  // do initialise

}

void L2Logic::remoteRequestLoop() {
  // Stall until the memory banks are capable of receiving a new flit.
  if (oRequestToBanks.valid()) {
    next_trigger(oRequestToBanks.ack_event());
  }
  else {
    loki_assert(requestsFromNetwork.canRead());
    NetworkRequest flit = requestsFromNetwork.read();

    LOKI_LOG << this->name() << " sending request to banks " << flit << endl;

    // For each new request, update the target bank. This bank services the
    // request in the event that no bank currently holds the required data.
    // The same target bank is used for all flits of a single request.
    if (newRemoteRequest) {
      MemoryIndex targetBank = getTargetBank(flit);
      oRequestTarget.write(targetBank);
    }

    oRequestToBanks.write(flit);

    newRemoteRequest = flit.getMetadata().endOfPacket;
  }
}

void L2Logic::requestClaimLoop() {
  bool claimed = false;
  for (uint i=0; i<iClaimRequest.size(); i++) {
    if (iClaimRequest[i].read()) {
      loki_assert_with_message(!claimed, "Multiple L2 banks responding to request for 0x%x.", oRequestToBanks.read().payload().toUInt());
      claimed = true;
    }
  }
  oRequestClaimed.write(claimed);
}

void L2Logic::requestDelayLoop() {
  bool delayed = false;
  for (uint i=0; i<iDelayRequest.size(); i++) {
    if (iDelayRequest[i].read()) {
      delayed = true;
    }
  }
  oRequestDelayed.write(delayed);
}

void L2Logic::sendResponseLoop() {
  if (!oResponseToNetwork->canWrite()) {
    next_trigger(oResponseToNetwork->canWriteEvent() & clock.posedge_event());
  }
  else if (muxedResponse.valid()) {
    oResponseToNetwork->write(muxedResponse.read());

    // Optional: if there is more data to come in this packet, wait until it
    // has all been sent before switching to another packet.
    holdResponseMux.write(!muxedResponse.read().getMetadata().endOfPacket);

    muxedResponse.ack();

    // Wait for the clock edge rather than the next data arrival because the
    // mux may have other data lined up and ready to go immediately.
    next_trigger(clock.posedge_event());
  }
}

MemoryIndex L2Logic::getTargetBank(const NetworkRequest& request) {
  // In scratchpad mode, the bank is specified by the bits immediately
  // above the offset.
  if (request.getMemoryMetadata().scratchpad) {
    return (request.payload().toUInt() >> log2CacheLineSize) & (numMemoryBanks - 1);
  }

  // Push line operations specify the target bank with the lowest bits.
  else if (request.getMemoryMetadata().opcode == PUSH_LINE) {
    return request.payload().toUInt() & (numMemoryBanks - 1);
  }

  // Ensure that requests which skip this memory cannot be reordered by
  // using the same bank mapping as in the L1. (returnChannel = bank).
  else if (request.getMemoryMetadata().skipL2) {
    return request.getMemoryMetadata().returnChannel;
  }

  // Otherwise, the target bank is random.
  else {
    return nextRandomBank();
  }
}

MemoryIndex L2Logic::nextRandomBank() {
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
  targetBank = (targetBank - 1) % numMemoryBanks;
  if (rngState & 0x1) {
    // Rotate right 1.
    targetBank = (targetBank - 1) % numMemoryBanks;
  }
  if (rngState & 0x4) {
    // Rotate right 4.
    targetBank = (targetBank - 4) % numMemoryBanks;
  }

  // Linear feedback shift register RNG. The generator polynomial used
  // here is x^6 + x^5 + 1. Has a period of 63. Do not change this
  // without evaluating the impact on the code above! PRNGs are hard and
  // this one has been experimentally tuned to yield a good period.
  rngState = (rngState >> 1) ^ ((rngState & 0x1) ? 0x30 : 0x0);

  return targetBank;
}

