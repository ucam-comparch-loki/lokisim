/*
 * EndArbiter.cpp
 *
 *  Created on: 6 Sep 2011
 *      Author: db434
 */

#include "EndArbiter.h"

const sc_event& EndArbiter::canGrantNow(int output) {
  // The sender knows when it's safe to send data, so we don't need to monitor
  // acks. If we also enforce that the sender only takes down their request
  // after the final ack has been received, we know that it is immediately safe
  // to grant a new request.

  grantEvent.notify(sc_core::SC_ZERO_TIME);
  return grantEvent;
}

EndArbiter::EndArbiter(const sc_module_name& name, ComponentID ID,
                       int inputs, int outputs, bool wormhole) :
    BasicArbiter(name, ID, inputs, outputs, wormhole) {
  // Nothing
}
