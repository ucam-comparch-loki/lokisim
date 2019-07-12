/*
 * L2RequestFilter.cpp
 *
 *  Created on: 6 Jul 2015
 *      Author: db434
 */

#include "../../Tile/Memory/L2RequestFilter.h"
#include "../../Tile/Memory/MemoryBank.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/Latency.h"

L2RequestFilter::L2RequestFilter(const sc_module_name& name, MemoryBank& localBank) :
    LokiComponent(name),
    iRequest("iRequest"),
    oRequest("oRequest"),
    localBank(localBank) {

  state = STATE_IDLE;

}

L2RequestFilter::~L2RequestFilter() {
}

void L2RequestFilter::end_of_elaboration() {
  SC_METHOD(mainLoop);
  sensitive << iRequest->newRequestArrived();
  dont_initialize();
}

void L2RequestFilter::mainLoop() {

  // The common case operation of the L2 request filter is:
  //  1. Request arrives from network
  //  2. All banks check cache tags simultaneously
  //  3a. If one bank has the data, it serves the request
  //  3b. If all banks miss, the randomly selected target takes responsibility
  //
  // For any request, at most one bank may have a cache hit.
  //
  // However, this becomes more complex when the cache is under load:
  //  * Never access cache tags if the output of this unit has not yet reached
  //    the memory bank. The memory bank will not have updated its state to
  //    account for that request.
  //  * To compensate for this backpressure, allow a bank with a cache hit to
  //    proceed before all banks have responded. (The Verilog doesn't do this.)
  //  * If no banks have a cache hit, must wait for all banks to respond before
  //    assigning the request to the random target bank.

  switch (state) {

    // Our first time seeing the request - check tags to see if we have the data,
    // otherwise check whether we are responsible on a miss.
    case STATE_IDLE: {
      // Wait until output is available.
      if (oRequest.valid()) {
        next_trigger(oRequest.ack_event());
        break;
      }
      // While waiting for output, the input might have already been claimed.
      else if (iRequest->associativeHit()) {
        next_trigger(iRequest->newRequestArrived());
        break;
      }

      const NetworkRequest& request = iRequest->peek();
      MemoryOpcode opcode = request.getMemoryMetadata().opcode;

      loki_assert((opcode != PAYLOAD) && (opcode != PAYLOAD_EOP));

      MemoryAddr address = request.payload().toUInt();

      // If this bank is currently waiting to flush the requested data, wait
      // until the flush completes before allowing any bank to fetch it.
      if (localBank.flushing(address)) {
        next_trigger(localBank.requestSentEvent());
        LOKI_LOG(3) << this->name() << " delaying L2 request because currently flushing " << LOKI_HEX(address) << endl;
        break;
      }

      MemoryAccessMode mode = (request.getMemoryMetadata().scratchpad ? MEMORY_SCRATCHPAD : MEMORY_CACHE);
      SRAMAddress position = localBank.getPosition(address, mode);

      // Perform a few checks to see whether this bank should claim the
      // request now, wait until next cycle, or ignore the request entirely.
      // This bank is responsible if the operation is one which specifies
      // that this bank should be used, or if the bank was chosen randomly
      // but this bank contains the data already.
      bool cacheHit = localBank.contains(address, position, mode);
      bool targetingThisBank = iRequest->targetBank() == localBank.memoryIndex();
      bool mustAccessTarget = (mode == MEMORY_SCRATCHPAD) || (opcode == PUSH_LINE) || request.getMemoryMetadata().skipL2;
      bool ignore = mustAccessTarget && !targetingThisBank;
      bool serveRequest = (targetingThisBank && mustAccessTarget) || (cacheHit && !ignore);

      // In order to account for requests which aren't in cache mode, we don't
      // actually send the cacheHit signal.
      if (serveRequest)
        iRequest->cacheHit();
      else
        iRequest->cacheMiss();

//        cout << this->name() << (mode == MEMORY_CACHE ? " cache access," : " scratchpad access,")
//                             << (cacheHit ? " cache hit," : "")
//                             << (targetingThisBank ? " is target," : "")
//                             << (ignore ? " ignoring request," : "")
//                             << (serveRequest ? " serving request" : "") << endl;

      if (serveRequest) {
        LOKI_LOG(2) << this->name() << " claiming request (cache hit)" << endl;

        state = STATE_SEND;
        next_trigger(sc_core::SC_ZERO_TIME);

        Instrumentation::Latency::memoryReceivedRequest(localBank.id, request);
      }
      else if (targetingThisBank) {
        // Wait in case anyone else claims.
        next_trigger(iRequest->allResponsesReceivedEvent());
        state = STATE_WAIT;
      }
      else {
        next_trigger(iRequest->newRequestArrived());
      }
      break;
    }

    // We have already checked the request and determined that we are responsible
    // if no one else already has the data.
    case STATE_WAIT:
      // Someone else claimed the request - wait for a new one.
      if (iRequest->associativeHit()) {
        state = STATE_IDLE;
        next_trigger(iRequest->newRequestArrived());
      }
      // No one else has claimed - the request is ours.
      else {
        LOKI_LOG(2) << this->name() << " claiming request (target bank)" << endl;

        state = STATE_SEND;
        next_trigger(sc_core::SC_ZERO_TIME);

        Instrumentation::Latency::memoryReceivedRequest(localBank.id, iRequest->peek());
      }
      break;

    // We have already claimed the request and have received a new flit.
    case STATE_SEND: {
      if (oRequest.valid()) {
        next_trigger(oRequest.ack_event());
        break;
      }

      NetworkRequest request = iRequest->read();
      oRequest.write(request);

      // If this was the final flit, stop claiming flits.
      if (request.getMetadata().endOfPacket)
        state = STATE_IDLE;

      next_trigger(iRequest->newFlitArrived());
      break;
    }

  } // end switch
}
