/*
 * L2RequestFilter.cpp
 *
 *  Created on: 6 Jul 2015
 *      Author: db434
 */

#include "../../Tile/Memory/L2RequestFilter.h"

#include "../../Tile/Memory/MemoryBank.h"
#include "../../Utility/Assert.h"

L2RequestFilter::L2RequestFilter(const sc_module_name& name, ComponentID id, MemoryBank* localBank) :
    LokiComponent(name, id),
    iClock("iClock"),
    iRequest("iRequest"),
    iRequestTarget("iRequestTarget"),
    oRequest("oRequest"),
    iRequestClaimed("iRequestClaimed"),
    oClaimRequest("oClaimRequest"),
    localBank(localBank) {

  state = STATE_IDLE;

  SC_METHOD(mainLoop);
  sensitive << iRequest << oRequest.ack_finder();

}

L2RequestFilter::~L2RequestFilter() {
}

void L2RequestFilter::mainLoop() {

  if (iRequest.valid()) {
    switch (state) {

      // Our first time seeing the request - check tags to see if we have the data,
      // otherwise check whether we are responsible on a miss.
      case STATE_IDLE: {
        NetworkRequest request = iRequest.read();
        MemoryOpcode opcode = request.getMemoryMetadata().opcode;

        loki_assert((opcode != PAYLOAD) && (opcode != PAYLOAD_EOP));

        MemoryAddr address = request.payload().toUInt();
        MemoryAccessMode mode = (request.getMemoryMetadata().scratchpad ? MEMORY_SCRATCHPAD : MEMORY_CACHE);
        SRAMAddress position = localBank->getPosition(address, mode);

        // Perform a few checks to see whether this bank should claim the
        // request now, wait until next cycle, or ignore the request entirely.
        // This bank is responsible if the operation is one which specifies
        // that this bank should be used, or if the bank was chosen randomly
        // but this bank contains the data already.
        bool cacheHit = localBank->contains(address, position, mode);
        bool targetingThisBank = iRequestTarget.read() == id.localMemoryNumber();
        bool mustAccessTarget = (mode == MEMORY_SCRATCHPAD) || (opcode == PUSH_LINE);
        bool ignore = mustAccessTarget && !targetingThisBank;
        bool serveRequest = (targetingThisBank && mustAccessTarget) || (cacheHit && !ignore);

//        cout << this->name() << (mode == MEMORY_CACHE ? " cache access," : " scratchpad access,")
//                             << (cacheHit ? " cache hit," : "")
//                             << (targetingThisBank ? " is target," : "")
//                             << (ignore ? " ignoring request," : "")
//                             << (serveRequest ? " serving request" : "") << endl;

        if (serveRequest) {
          oClaimRequest.write(true);
          oRequest.write(iRequest.read());
          state = STATE_ACKNOWLEDGE;
        }
        else if (targetingThisBank) {
          // Wait a clock cycle in case anyone else claims.
          next_trigger(iClock.posedge_event() | iRequestClaimed.posedge_event());
          state = STATE_WAIT;
        }
        else {
          next_trigger(iRequestClaimed.negedge_event());
        }
        break;
      }

      // We have already checked the request and determined that we are responsible
      // if no one else already has the data.
      case STATE_WAIT:
        // Someone else claimed the request - wait for a new one.
        if (iRequestClaimed.read()) {
          next_trigger(iRequestClaimed.negedge_event());
          state = STATE_IDLE;
        }
        // No one else has claimed - the request is ours.
        else {
          oClaimRequest.write(true);
          oRequest.write(iRequest.read());
          state = STATE_ACKNOWLEDGE;
        }
        break;

      // We have already claimed the request and have received a new flit.
      case STATE_SEND:
        oRequest.write(iRequest.read());
        state = STATE_ACKNOWLEDGE;
        break;

      // The request is old - it was forwarded to the memory bank and has now been
      // consumed.
      case STATE_ACKNOWLEDGE: {
        NetworkRequest request = iRequest.read();
        iRequest.ack();

        // If this was the final flit, stop claiming flits.
        if (request.getMetadata().endOfPacket) {
          oClaimRequest.write(false);
          state = STATE_IDLE;
        }
        else
          state = STATE_SEND;

        break;
      }
    } // end switch
  }
  else // Invalid request
    next_trigger(iRequest.default_event());
}

