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
    iClock("iClock"),
    iRequest("iRequest"),
    oRequest("oRequest"),
    l2Associativity("l2Associativity"),
    localBank(localBank),
    inBuffer("inBuffer", 1) {

  iRequest(inBuffer);

  state = STATE_IDLE;

  SC_METHOD(mainLoop);
  sensitive << inBuffer.canReadEvent();
  dont_initialize();

}

L2RequestFilter::~L2RequestFilter() {
}

void L2RequestFilter::mainLoop() {

  if (inBuffer.canRead()) {
    switch (state) {

      // Our first time seeing the request - check tags to see if we have the data,
      // otherwise check whether we are responsible on a miss.
      case STATE_IDLE: {
        const NetworkRequest& request = inBuffer.peek();
        MemoryOpcode opcode = request.getMemoryMetadata().opcode;

        loki_assert((opcode != PAYLOAD) && (opcode != PAYLOAD_EOP));

        MemoryAddr address = request.payload().toUInt();
        MemoryAccessMode mode = (request.getMemoryMetadata().scratchpad ? MEMORY_SCRATCHPAD : MEMORY_CACHE);
        SRAMAddress position = localBank.getPosition(address, mode);

        // Perform a few checks to see whether this bank should claim the
        // request now, wait until next cycle, or ignore the request entirely.
        // This bank is responsible if the operation is one which specifies
        // that this bank should be used, or if the bank was chosen randomly
        // but this bank contains the data already.
        bool cacheHit = localBank.contains(address, position, mode);
        bool targetingThisBank = l2Associativity->targetBank() == localBank.memoryIndex();
        bool mustAccessTarget = (mode == MEMORY_SCRATCHPAD) || (opcode == PUSH_LINE) || request.getMemoryMetadata().skipL2;
        bool ignore = mustAccessTarget && !targetingThisBank;
        bool serveRequest = (targetingThisBank && mustAccessTarget) || (cacheHit && !ignore);

        // In order to account for requests which aren't in cache mode, we don't
        // actually send the cacheHit signal.
        if (serveRequest)
          l2Associativity->cacheHit();
        else
          l2Associativity->cacheMiss();

//        cout << this->name() << (mode == MEMORY_CACHE ? " cache access," : " scratchpad access,")
//                             << (cacheHit ? " cache hit," : "")
//                             << (targetingThisBank ? " is target," : "")
//                             << (ignore ? " ignoring request," : "")
//                             << (serveRequest ? " serving request" : "") << endl;

        if (serveRequest) {
          LOKI_LOG << this->name() << " claiming request (cache hit)" << endl;

          state = STATE_SEND;
          next_trigger(sc_core::SC_ZERO_TIME);

          Instrumentation::Latency::memoryReceivedRequest(localBank.id, request);
        }
        else if (targetingThisBank) {
          // Wait in case anyone else claims.
          next_trigger(l2Associativity->allResponsesReceivedEvent());
          state = STATE_WAIT;
        }
        else {
          state = STATE_CONSUME;
          next_trigger(sc_core::SC_ZERO_TIME);
        }
        break;
      }

      // We have already checked the request and determined that we are responsible
      // if no one else already has the data.
      case STATE_WAIT:
        // Someone else claimed the request - discard this one and wait for a
        // new one.
        if (l2Associativity->associativeHit()) {
          state = STATE_CONSUME;
          next_trigger(sc_core::SC_ZERO_TIME);
        }
        // No one else has claimed - the request is ours.
        else {
          LOKI_LOG << this->name() << " claiming request (target bank)" << endl;

          state = STATE_SEND;
          next_trigger(sc_core::SC_ZERO_TIME);

          Instrumentation::Latency::memoryReceivedRequest(localBank.id, inBuffer.peek());
        }
        break;

      // We have already claimed the request and have received a new flit.
      case STATE_SEND: {
        loki_assert(!oRequest.valid());
        loki_assert(inBuffer.canRead());

        NetworkRequest request = inBuffer.read();
        oRequest.write(request);

        // If this was the final flit, stop claiming flits.
        if (request.getMetadata().endOfPacket)
          state = STATE_IDLE;

        next_trigger(oRequest.ack_event());
        break;
      }

      // Consume flits from a request we are not involved in.
      case STATE_CONSUME: {
        const NetworkRequest& request = inBuffer.read();

        // Stop consuming when we reach the end of the packet.
        if (request.getMetadata().endOfPacket)
          state = STATE_IDLE;

        break;
      }

    } // end switch
  }
  else // Invalid request
    next_trigger(inBuffer.canReadEvent());
}
