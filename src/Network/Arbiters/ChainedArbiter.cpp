/*
 * ChainedArbiter.cpp
 *
 *  Created on: 6 Sep 2011
 *      Author: db434
 */

#include "ChainedArbiter.h"

const sc_event& ChainedArbiter::canGrantNow(int output, const ChannelIndex destination) {
  // Send a request to the next arbiter in the chain.
  requestOut[output].write(destination);

  // We know it is safe to send data when we get the grant back.
  return grantIn[output].posedge_event();
}

const sc_event& ChainedArbiter::stallGrant(int output) {
  // We must stop granting requests if the next arbiter removes its grant.
  // This probably means the destination buffer is full.

  return grantIn[output].negedge_event();
}

void ChainedArbiter::deassertGrant(int input, int output) {
  // Do everything that parent class does.
  BasicArbiter::deassertGrant(input, output);

  // As well as deasserting the grant, we need to deassert the request to the
  // next arbiter.
  requestOut[output].write(NO_REQUEST);
}

ChainedArbiter::ChainedArbiter(const sc_module_name& name, ComponentID ID,
                               int inputs, int outputs, bool wormhole) :
    BasicArbiter(name, ID, inputs, outputs, wormhole) {

  requestOut.init(outputs);
  grantIn.init(outputs);

}
