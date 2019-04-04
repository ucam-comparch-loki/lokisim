/*
 * L2Logic.cpp
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#include "L2Logic.h"
#include "../../Utility/Assert.h"

L2Logic::L2Logic(const sc_module_name& name, const tile_parameters_t& params) :
    LokiComponent(name),
    clock("clock"),
    iRequestFromNetwork("iRequestFromNetwork"),
    oResponseToNetwork("oResponseToNetwork"),
    iResponseFromBanks("iResponseFromBanks"),
    oRequestToBanks("oRequestToBanks"),
    log2CacheLineSize(params.memory.log2CacheLineSize()),
    numMemoryBanks(params.numMemories),
    requestsFromNetwork("requestsFromNetwork", 1) {

  rngState = 0x3F;  // Same seed as Verilog uses (see nextTargetBank()).
  randomBank = 0;
  newRemoteRequest = true;

  iRequestFromNetwork(requestsFromNetwork);
  iResponseFromBanks(oResponseToNetwork); // Pass through, do nothing.

  SC_METHOD(forwardRequests);
  sensitive << requestsFromNetwork.canReadEvent();
  dont_initialize();

}

void L2Logic::forwardRequests() {
  if (!oRequestToBanks->canWrite())
    next_trigger(oRequestToBanks->canWriteEvent());
  else {
    NetworkRequest flit = requestsFromNetwork.read();

    // For each new request, update the target bank. This bank services the
    // request in the event that no bank currently holds the required data.
    // The same target bank is used for all flits of a single request.
    if (newRemoteRequest) {
      MemoryIndex targetBank = getTargetBank(flit);
      oRequestToBanks->newRequest(flit, targetBank);
    }
    else
      oRequestToBanks->newPayload(flit);

    newRemoteRequest = flit.getMetadata().endOfPacket;
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

  // Just using the state on its own to generate the target leads to
  // uneven bank choice (note the period is not a multiple of 8!). The
  // following technique has been confirmed experimentally to lead to a
  // period of num_banks * 63 for 1 <= num_banks <= 32.
  //
  // Admittedly this verilog breaks for num_banks <= 2, but you get the
  // idea.
  //
  // Rotate right 1.
  randomBank = (randomBank - 1) % numMemoryBanks;
  if (rngState & 0x1) {
    // Rotate right 1.
    randomBank = (randomBank - 1) % numMemoryBanks;
  }
  if (rngState & 0x4) {
    // Rotate right 4.
    randomBank = (randomBank - 4) % numMemoryBanks;
  }

  // Linear feedback shift register RNG. The generator polynomial used
  // here is x^6 + x^5 + 1. Has a period of 63. Do not change this
  // without evaluating the impact on the code above! PRNGs are hard and
  // this one has been experimentally tuned to yield a good period.
  rngState = (rngState >> 1) ^ ((rngState & 0x1) ? 0x30 : 0x0);

  return randomBank;
}

