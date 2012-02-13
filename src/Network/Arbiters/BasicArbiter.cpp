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
        changeSelection(output, NO_SELECTION);
        next_trigger(receivedRequest);
        break;
      }
    }

    // We have requests, and it is time to perform the arbitration.
    case HAVE_REQUESTS: {
      SelectType grant = arbiter->getGrant();
      changeSelection(output, grant);

      // Remove the granted request from consideration.
      requestVec[grant] = false;

      if(grant != ArbiterBase::NO_GRANT) {    // Successful grant
        assert(grant < inputs);

        // Update the internal array so that no one else issues the same grant,
        // but don't change the signal until we are sure that it is possible to
        // send the data.
        grantVec[grant] = true;

        state[output] = WAITING_TO_GRANT;

        next_trigger(canGrantNow(output, requests[grant].read()));
      }
      else {
        // If we couldn't grant anything, wait until next cycle.
        state[output] = NO_REQUESTS;
        next_trigger(clock.negedge_event());
      }
      break;
    }

    // We have determined which request to grant, but haven't yet been able
    // to send the grant. Send it now.
    case WAITING_TO_GRANT: {
      SelectType granted = selectVec[output];
      assert(granted < inputs);
      assert(granted >= 0);

      bool requestStillActive = (requests[granted].read() != NO_REQUEST);

      if(requestStillActive) {
        grant(granted, output);

        // Wait until the request line is deasserted. This means all flits in
        // the packet have been sent. (Make this wormhole behaviour optional?)
        // Alternatively, a flow control signal may force us to stall the
        // sending of data.
        state[output] = GRANTED;
        next_trigger(requests[granted].default_event() | stallGrant(output));
      }
      else {
        deassertGrant(granted, output);
        state[output] = NO_REQUESTS;
        next_trigger(clock.negedge_event());
      }

      break;
    }

    // We have sent the grant and the request has been removed. Remove the
    // grant now.
    case GRANTED: {
      SelectType granted = selectVec[output];
      assert(granted < inputs);
      assert(granted != NO_SELECTION);

      if(requests[granted].read() == NO_REQUEST) {
        // The request was removed, meaning the communication completed
        // successfully.
        deassertGrant(granted, output);

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
      }
      else {
        // The destination cannot receive any more data at the moment. Wait
        // until it is ready again (or until the request is removed).
        grantVec[granted] = false;
        grantChanged[granted].notify();

        state[output] = WAITING_TO_GRANT;
        next_trigger(canGrantNow(output, requests[granted].read()) |
                     requests[granted].default_event());
      }

      break;
    }
  } // end switch
}

void BasicArbiter::grant(int input, int output) {
  grantVec[input] = true;
  grantChanged[input].notify();

  changeSelection(output, input);
}

void BasicArbiter::deassertGrant(int input, int output) {
  grantVec[input] = false;
  grantChanged[input].notify();

  changeSelection(output, NO_SELECTION);
}

void BasicArbiter::changeSelection(int output, SelectType value) {
  if(value != selectVec[output]) {
    selectVec[output] = value;
    selectionChanged[output].notify();
  }
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
  requestVec[input] = (requests[input].read() != NO_REQUEST);

  if(requestVec[input])
    receivedRequest.notify();
}

void BasicArbiter::updateGrant(int input) {
  // Update the output signal to the value held in the internal vector.
  grants[input].write(grantVec[input]);
}

void BasicArbiter::updateSelect(int output) {
  // Wait until the start of the next cycle before changing the select signal.
  // Arbitration is done the cycle before data is sent.
  if(clock.posedge()) {   // The clock edge
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
    requestVec(inputs, false),
    grantVec(inputs, false),
    selectVec(outputs, -1),
    state(outputs, NO_REQUESTS) {

  requests = new RequestInput[inputs];
  grants   = new GrantOutput[inputs];
  select   = new SelectOutput[outputs];

  grantChanged = new sc_event[inputs];
  selectionChanged = new sc_event[outputs];

  // Set some initial value for the select signals, so they generate an event
  // whenever they change to a valid value.
  for(int i=0; i<outputs; i++) select[i].initialize(NO_SELECTION);

  arbiter = ArbiterBase::makeArbiter(ArbiterBase::ROUND_ROBIN, &requestVec, &grantVec);

  // Generate a method for each output port, granting new requests whenever
  // the output becomes free.
  for(int i=0; i<outputs; i++)
    SPAWN_METHOD(receivedRequest, BasicArbiter::arbitrate, i, false);

  // Generate a method to watch each request port, taking appropriate action
  // whenever the signal changes.
  for(int i=0; i<inputs; i++)
    SPAWN_METHOD(requests[i], BasicArbiter::requestChanged, i, false);

  // Method for each grant port, updating the grant signal when appropriate.
  for(int i=0; i<inputs; i++)
    SPAWN_METHOD(grantChanged[i], BasicArbiter::updateGrant, i, false);

  // Method for each select port, updating the signal when appropriate.
  for(int i=0; i<outputs; i++)
    SPAWN_METHOD(selectionChanged[i], BasicArbiter::updateSelect, i, false);

}

BasicArbiter::~BasicArbiter() {
  delete[] requests;
  delete[] grants;
  delete[] select;

  delete[] grantChanged;
  delete[] selectionChanged;

  delete arbiter;
}
