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
  switch(state[output]) {
    // We had no requests, but now that this method has been called again, we
    // should have at least one. Wait until the clock edge to arbitrate.
    case NO_REQUESTS: {
      if(haveRequest()) {
        state[output] = HAVE_REQUESTS;

        // Fall through to HAVE_REQUESTS case.
      }
      else {
        changeSelection(output, -1);
        next_trigger(receivedRequest);
        break;
      }
    }

    // We have requests, and it is time to perform the arbitration.
    case HAVE_REQUESTS: {
      int grant = arbiter->getGrant();
      changeSelection(output, grant);

      if(grant != ArbiterBase::NO_GRANT) {    // Successful grant
        assert(grant < inputs);

        // Update the internal array so that no one else issues the same grant,
        // but don't change the signal until we are sure that it is possible to
        // send the data.
        grantVec[grant] = true;
        cout << this->name() << " output " << output << " is now taken\n";
        cout << this->name() << " granted request from input " << grant << " at " << sc_core::sc_simulation_time() << endl;

        state[output] = WAITING_TO_GRANT;
        next_trigger(canGrantNow(output));
      }
      else {
        if(haveRequest()) cout << this->name() << " has request but can't grant it\n";
        // If we couldn't grant anything, wait until next cycle.
        state[output] = NO_REQUESTS;
        next_trigger(clock.negedge_event());
      }
      break;
    }

    // We have determined which request to grant, but haven't yet been able
    // to send the grant. Send it now.
    case WAITING_TO_GRANT: {
      int granted = selectVec[output];
      assert(granted < inputs);
      assert(granted >= 0);

      requestGranted[granted].notify();

      cout << this->name() << " sent grant for input " << granted << " at " << sc_core::sc_simulation_time() << endl;

      // Wait until the request line is deasserted. This means all flits in
      // the packet have been sent. (Make this wormhole behaviour optional?)
      state[output] = GRANTED;
      next_trigger(requests[granted].negedge_event());

      break;
    }

    // We have sent the grant and the request has been removed. Remove the
    // grant now.
    case GRANTED: {
      int granted = selectVec[output];
      assert(granted < inputs);
      assert(granted >= 0);

      deassertGrant(granted, output);
      cout << this->name() << " output " << output << " is now available\n";

      if(haveRequest()) {
        // Wait until next cycle to try and grant a new request.
        state[output] = HAVE_REQUESTS;
        next_trigger(clock.negedge_event());
      }
      else {
        // Wait until a request is received.
        state[output] = NO_REQUESTS;
        next_trigger(clock.negedge_event());
      }

      break;
    }
  } // end switch
}

void BasicArbiter::deassertGrant(int input, int output) {
  grantVec[input] = false;
  requestGranted[input].notify();

  changeSelection(output, -1);
}

void BasicArbiter::changeSelection(int output, int value) {
  selectVec[output] = value;
  selectionChanged[output].notify();
}

bool BasicArbiter::haveRequest() const {
  // A simple loop through the vector for now. If this proves to be too
  // expensive, could keep a separate count.
  for(int i=0; i<inputs; i++) {
    if(requestVec[i] && !grantVec[i]) return true;
  }

  // No requests were found in the vector.
  return false;
}

void BasicArbiter::requestChanged(int input) {
  requestVec[input] = requests[input].read();

  if(requestVec[input]) {
    cout << this->name() << " received request at input " << input << " at " << sc_core::sc_simulation_time() << endl;
    receivedRequest.notify();
  }
}

void BasicArbiter::updateGrant(int input) {
  // Update the output signal to the value held in the internal vector.
  grants[input].write(grantVec[input]);
}

void BasicArbiter::updateSelect(int output) {
  // Wait until the start of the next cycle before changing the select signal.
  // Arbitration is done the cycle before data is sent.
  // We actually write the signal a fraction after the clock edge to avoid
  // race conditions with other signals.
  if(clock.posedge()) {   // The clock edge
    cout << this->name() << " wrote new select value: " << selectVec[output] << endl;
    select[output].write(selectVec[output]);
    next_trigger(selectionChanged[output]);
  }
  else {                  // Select signal changed
    next_trigger(clock.posedge_event());
  }
}

BasicArbiter::BasicArbiter(const sc_module_name& name, ComponentID ID,
                           int inputs, int outputs, bool wormhole) :
    Component(name, ID),
    inputs(inputs),
    outputs(outputs),
    wormhole(wormhole),
    state(outputs, NO_REQUESTS),
    requestVec(inputs, false),
    grantVec(inputs, false),
    selectVec(outputs, -1) {

  requests = new sc_in<bool>[inputs];
  grants   = new sc_out<bool>[inputs];
  select   = new sc_out<int>[outputs];

  requestGranted = new sc_event[inputs];
  selectionChanged = new sc_event[outputs];

  // Set some initial value for the select signals, so they generate an event
  // whenever they change to a valid value.
  for(int i=0; i<outputs; i++) select[i].initialize(-1);

  arbiter = ArbiterBase::makeArbiter(ArbiterBase::ROUND_ROBIN, &requestVec, &grantVec);

  // Generate a method for each output port, granting new requests whenever
  // the output becomes free.
  for(int i=0; i<outputs; i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread
    options.dont_initialize();  // Only execute when triggered
    options.set_sensitivity(&receivedRequest);

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

  // Method for each grant port, updating the grant signal when appropriate.
  for(int i=0; i<inputs; i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread
    options.dont_initialize();  // Only execute when triggered
    options.set_sensitivity(&(requestGranted[i]));

    // Create the method.
    sc_spawn(sc_bind(&BasicArbiter::updateGrant, this, i), 0, &options);
  }

  // Method for each select port, updating the signal when appropriate.
  for(int i=0; i<outputs; i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread
    options.dont_initialize();  // Only execute when triggered
    options.set_sensitivity(&(selectionChanged[i]));

    // Create the method.
    sc_spawn(sc_bind(&BasicArbiter::updateSelect, this, i), 0, &options);
  }

}

BasicArbiter::~BasicArbiter() {
  delete[] requests;
  delete[] grants;
  delete[] select;

  delete[] requestGranted;
  delete[] selectionChanged;

  delete arbiter;
}
