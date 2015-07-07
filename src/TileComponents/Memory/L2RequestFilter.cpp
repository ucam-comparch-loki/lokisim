/*
 * L2RequestFilter.cpp
 *
 *  Created on: 6 Jul 2015
 *      Author: db434
 */

#include "L2RequestFilter.h"
#include "MemoryBank.h"

L2RequestFilter::L2RequestFilter(const sc_module_name& name, ComponentID id, MemoryBank* localBank) :
    Component(name, id),
    localBank(localBank) {

  state = STATE_IDLE;

  SC_METHOD(mainLoop);
  sensitive << iRequest << oRequest.ack_finder();
  dont_initialize();

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

        assert(request.getMemoryMetadata().opcode != MemoryRequest::PAYLOAD_ONLY);

        MemoryAddr address = request.payload().toUInt();
        bool cacheHit = localBank->storedLocally(address);
        if (cacheHit) {
          oClaimRequest.write(true);
          oRequest.write(iRequest.read());
          state = STATE_ACKNOWLEDGE;
        }
        else if (iRequestTarget.read() == (id.position - CORES_PER_TILE)) {
          // Wait a clock cycle in case anyone else claims.
          next_trigger(iClock.negedge_event());
          state = STATE_WAIT;
        }
        else
          next_trigger(iRequestClaimed.negedge_event());
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
}

