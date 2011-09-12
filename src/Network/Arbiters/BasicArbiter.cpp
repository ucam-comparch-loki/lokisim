/*
 * BasicArbiter.cpp
 *
 *  Created on: 6 Sep 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "BasicArbiter.h"
#include "../../Arbitration/ArbiterBase.h"

int BasicArbiter::numInputs()  const {return inputs;}
int BasicArbiter::numOutputs() const {return outputs;}

void BasicArbiter::arbitrate(int output) {
  // Arbitration happens on the negative edge (is this still necessary?).
  if(clock.negedge()) {
    if(!haveRequest()) {
      next_trigger(receivedRequest);
      return;
    }

    int grant = arbiter->getGrant();
    if(grant != ArbiterBase::NO_GRANT) {
      assert(grant < inputs);

      // Update the internal array so that no one else issues the same grant,
      // but don't change the signal until we are sure that it is possible to
      // send the data.
      grantVec[grant] = true;
      select[output].write(grant);

      next_trigger(canGrantNow(output));
    }
    else {
      // If we couldn't grant anything, wait until next cycle.
      next_trigger(clock.negedge_event());
    }
  }
  // Everything that isn't arbitration happens elsewhere in the cycle.
  else {
    int granted = select[output].read();

    if(grants[granted].read()) {
      // We have sent the grant: deassert it now.
      deassertGrant(granted, output);

      // Wait until next cycle to try and grant a new request.
      next_trigger(clock.negedge_event());
    }
    else {
      // We haven't sent the grant yet: send it now.
      grants[granted].write(true);

      // Wait until the request line is deasserted. This means all flits in
      // the packet have been sent. (Make this wormhole behaviour optional?)
      next_trigger(requests[granted].negedge_event());
    }
  }
}

void BasicArbiter::deassertGrant(int input, int output) {
  grants[input].write(false);
  grantVec[input] = false;
}

bool BasicArbiter::haveRequest() const {
  // A simple loop through the vector for now. If this proves to be too
  // expensive, could keep a separate count.
  for(int i=0; i<inputs; i++) {
    if(requestVec[i]) return true;
  }

  // No requests were found in the vector.
  return false;
}

void BasicArbiter::requestChanged(int input) {
  requestVec[input] = requests[input].read();
  if(requestVec[input]) receivedRequest.notify();
}

BasicArbiter::BasicArbiter(const sc_module_name& name, ComponentID ID,
                           int inputs, int outputs, bool wormhole) :
    Component(name, ID),
    inputs(inputs),
    outputs(outputs),
    wormhole(wormhole),
    requestVec(inputs, false),
    grantVec(inputs, false) {

  requests = new sc_in<bool>[inputs];
  grants   = new sc_out<bool>[inputs];
  select   = new sc_out<int>[outputs];

  arbiter = ArbiterBase::makeArbiter(ArbiterBase::ROUND_ROBIN, &requestVec, &grantVec);

  // Generate a method for each output port, granting new requests whenever
  // the output becomes free.
  for(int i=0; i<inputs; i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread
    options.dont_initialize();  // Only execute when triggered
    options.set_sensitivity(&(clock.negedge_event()));

    // Create the method.
    sc_spawn(sc_bind(&BasicArbiter::arbitrate, this, i), 0, &options);
  }

  // Generate a method to watch each request port, taking appropriate action
  // whenever the signal changes.
  for(int i=0; i<inputs; i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread
    options.dont_initialize();  // Only execute when triggered
    options.set_sensitivity(&(requests[i])); // Sensitive to this port

    // Create the method.
    sc_spawn(sc_bind(&BasicArbiter::requestChanged, this, i), 0, &options);
  }

}

BasicArbiter::~BasicArbiter() {
  delete[] requests;
  delete[] grants;
  delete[] select;

  delete arbiter;
}
