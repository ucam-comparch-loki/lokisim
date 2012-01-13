/*
 * EndArbiter.cpp
 *
 *  Created on: 6 Sep 2011
 *      Author: db434
 */

#include "EndArbiter.h"

const sc_event& EndArbiter::canGrantNow(int output, const ChannelIndex destination) {
  ChannelIndex channel;

  // Determine which "ready" signal to check. Memories only have one, but cores
  // have one for each buffer.
  if(outputs == 1)
    channel = 0;
  else
    channel = destination;

  // If the destination is ready to receive data, we can send the grant
  // immediately.
  if(readyIn[channel].read()) {
    grantEvent.notify(sc_core::SC_ZERO_TIME);
    return grantEvent;
  }
  // Otherwise, we must wait until the destination is ready.
  else {
    return readyIn[channel].posedge_event();
  }
}

const sc_event& EndArbiter::stallGrant(int output) {
  // We must stop granting requests if the destination component stops being
  // ready to receive more data.

  return readyIn[output].negedge_event();
}

// Override BasicArbiter's implementation so the request is only officially
// received when the output is free to receive data.
void EndArbiter::requestChanged(int input) {
  if(requests[input].read() == NO_REQUEST) {
    requestVec[input] = false;
  }
  else {
    PortIndex outputWanted = outputToUse(input);

    if(readyIn[outputWanted].read()) {
      requestVec[input] = true;
      receivedRequest.notify();
    }
    else {
      next_trigger(readyIn[outputWanted].posedge_event());
    }
  }
}

PortIndex EndArbiter::outputToUse(PortIndex input) {
  // Memories only have one buffer and one ready signal which cover all channels.
  // Cores have separate buffers and separate ready signals.

  if(outputs == 1)
    return 0;
  else
    return requests[input].read();
}

EndArbiter::EndArbiter(const sc_module_name& name, ComponentID ID,
                       int inputs, int outputs, bool wormhole) :
    BasicArbiter(name, ID, inputs, outputs, wormhole) {

  // FIXME: would prefer this as a constructor argument.
  // If this arbiter is connected to a core, there will be more ready signals
  // than there are data signals. There is one ready signal for each buffer.
  int readyInputs;
  if(outputs == 1) readyInputs = 1;
  else             readyInputs = CORE_INPUT_CHANNELS;

  readyIn = new ReadyInput[readyInputs];

}

EndArbiter::~EndArbiter() {
  delete[] readyIn;
}
