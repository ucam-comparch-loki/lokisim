/*
 * EndArbiter.cpp
 *
 *  Created on: 6 Sep 2011
 *      Author: db434
 */

#include "EndArbiter.h"

const sc_event& EndArbiter::canGrantNow(int output) {
  // If the destination is ready to receive data, we can send the grant
  // immediately.
  if(readyIn[output].read()) {
    grantEvent.notify(sc_core::SC_ZERO_TIME);
    return grantEvent;
  }
  // Otherwise, we must wait until the destination is ready.
  else {
    return readyIn[output].posedge_event();
  }
}

const sc_event& EndArbiter::stallGrant(int output) {
  // We must stop granting requests if the destination component stops being
  // ready to receive more data.

  return readyIn[output].negedge_event();
}

EndArbiter::EndArbiter(const sc_module_name& name, ComponentID ID,
                       int inputs, int outputs, bool wormhole) :
    BasicArbiter(name, ID, inputs, outputs, wormhole) {

  readyIn = new sc_in<bool>[outputs];

}

EndArbiter::~EndArbiter() {
  delete[] readyIn;
}
