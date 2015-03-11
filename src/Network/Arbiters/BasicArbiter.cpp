/*
 * BasicArbiter.cpp
 *
 *  Created on: 6 Sep 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "BasicArbiter.h"
#include "../../Arbitration/ArbiterBase.h"
#include "../../Utility/Instrumentation/Network.h"

int BasicArbiter::numInputs()  const {return iRequest.length();}
int BasicArbiter::numOutputs() const {return oSelect.length();}

void BasicArbiter::arbitrate(int output) {

  switch (state[output]) {
    // We had no requests, but now that this method has been called again, we
    // should have at least one. Wait until the clock edge to arbitrate.
    case NO_REQUESTS: {
      if (haveRequest()) {
        state[output] = HAVE_REQUESTS;

        // Fall through to HAVE_REQUESTS case.
      }
      else {
//        cout << this->name() << " waiting for requests" << endl;
        changeSelection(output, NO_SELECTION);
        next_trigger(receivedRequest);
        break;
      }
    }
    // no break

    // We have requests, and it is time to perform the arbitration.
    case HAVE_REQUESTS: {
//      cout << this->name() << " arbitrating" << endl;
      MuxSelect grant = arbiter->getGrant();
      selectVec[output] = grant;

      if (ENERGY_TRACE)
        Instrumentation::Network::arbitration();

      if (grant != ArbiterBase::NO_GRANT) {    // Successful grant
        assert(grant < numInputs());

//        cout << this->name() << " selected " << (int)grant << "; waiting for flow control" << endl;

        // Remove the granted request from consideration.
        requestVec[grant] = false;

        // Update the internal array so that no one else issues the same grant,
        // but don't change the signal until we are sure that it is possible to
        // send the data.
        grantVec[grant] = true;

        state[output] = WAITING_TO_GRANT;

        next_trigger(canGrantNow(output, iRequest[grant].read()));
      }
      else {
        // If we couldn't grant anything, wait until next cycle.
//        cout << this->name() << " unable to grant any requests" << endl;
        state[output] = NO_REQUESTS;
        next_trigger(clock.negedge_event());
      }

      break;
    }

    // We have determined which request to grant, but haven't yet been able
    // to send the grant. Send it now.
    case WAITING_TO_GRANT: {
      MuxSelect granted = selectVec[output];
      assert(granted < numInputs());
      assert(granted >= 0);

      bool requestStillActive = (iRequest[granted].read() != NO_REQUEST);

      if (requestStillActive) {
//        cout << this->name() << " ready to grant input " << (int)granted << endl;
        grant(granted, output);

        // Wait until the request line is deasserted. This means all flits in
        // the packet have been sent. (Make this wormhole behaviour optional?)
        // Alternatively, a flow control signal may force us to stall the
        // sending of data.
        state[output] = GRANTED;
        next_trigger(iRequest[granted].default_event() | stallGrant(output));
      }
      else {
//        cout << this->name() << " request at input " << (int)granted << " removed" << endl;
        deassertGrant(granted, output);
        state[output] = NO_REQUESTS;
        next_trigger(clock.negedge_event());
      }

      break;
    }

    // We have sent the grant and the request has been removed. Remove the
    // grant now.
    case GRANTED: {
      MuxSelect granted = selectVec[output];
      assert(granted < numInputs());
      assert(granted != NO_SELECTION);

      if (iRequest[granted].read() == NO_REQUEST) {
        // The request was removed, meaning the communication completed
        // successfully.
        deassertGrant(granted, output);
//        cout << this->name() << " removing grant for input " << (int)granted << endl;

        state[output] = haveRequest() ? HAVE_REQUESTS : NO_REQUESTS;

        next_trigger(clock.negedge_event());
      }
      else {
        // The destination cannot receive any more data at the moment. Wait
        // until it is ready again (or until the request is removed).
        grantVec[granted] = false;
        grantChanged[granted].notify();

//        cout << this->name() << " new request on input " << (int)granted << endl;

        state[output] = WAITING_TO_GRANT;
        next_trigger(canGrantNow(output, iRequest[granted].read()) |
                     iRequest[granted].default_event());
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

void BasicArbiter::changeSelection(int output, MuxSelect value) {
//  if(value != selectVec[output]) {
    selectVec[output] = value;
    selectionChanged[output].notify();
//  }
}

bool BasicArbiter::haveRequest() const {
  // A simple loop through the vector for now. If this proves to be too
  // expensive, could keep a separate count.
  for (int i=0; i<numInputs(); i++) {
    if (requestVec[i] && !grantVec[i]) return true;
  }

  // No requests were found in the vector.
  return false;
}

void BasicArbiter::requestChanged(int input) {
  requestVec[input] = (iRequest[input].read() != NO_REQUEST);

//  cout << this->name() << " request " << input << " changed to " << (int)(requestVec[input]) << endl;

  if (requestVec[input]) {
    receivedRequest.notify();
  }
}

void BasicArbiter::updateGrant(int input) {
  // Update the output signal to the value held in the internal vector.
  oGrant[input].write(grantVec[input]);
}

void BasicArbiter::updateSelect(int output) {
  // Wait until the start of the next cycle before changing the select signal.
  // Arbitration is done the cycle before data is sent.
  if (clock.posedge()) {  // The clock edge
    oSelect[output].write(selectVec[output]);
//    cout << this->name() << " granted input " << (int)(selectVec[output]) << endl;
    next_trigger(selectionChanged[output]);
  }
  else {                  // Select signal changed
    next_trigger(clock.posedge_event());
  }
}

void BasicArbiter::reportStalls(ostream& os) {
  for (uint i=0; i<iRequest.length(); i++) {
    if (requestVec[i]) {
      os << this->name() << " still has active request on input " << i << endl;
    }
  }
}

BasicArbiter::BasicArbiter(const sc_module_name& name, ComponentID ID,
                           int inputs, int outputs, bool wormhole) :
    Component(name, ID),
    wormhole(wormhole),
    requestVec(inputs, false),
    grantVec(inputs, false),
    selectVec(outputs, -1),
    state(outputs, NO_REQUESTS) {

  assert(inputs > 0);
  assert(outputs > 0);

  Instrumentation::Network::arbiterCreated();

  iRequest.init(inputs);
  oGrant.init(inputs);
  oSelect.init(outputs);

  grantChanged.init(inputs);
  selectionChanged.init(outputs);

  // Set some initial value for the select signals, so they generate an event
  // whenever they change to a valid value.
  for (int i=0; i<outputs; i++)
    oSelect[i].initialize(NO_SELECTION);

  arbiter = ArbiterBase::makeArbiter(ArbiterBase::ROUND_ROBIN, &requestVec, &grantVec);

  // Generate a method for each output port, granting new requests whenever
  // the output becomes free.
  for (int i=0; i<outputs; i++)
    SPAWN_METHOD(receivedRequest, BasicArbiter::arbitrate, i, false);

  // Generate a method to watch each request port, taking appropriate action
  // whenever the signal changes.
  for (int i=0; i<inputs; i++)
    SPAWN_METHOD(iRequest[i], BasicArbiter::requestChanged, i, false);

  // Method for each grant port, updating the grant signal when appropriate.
  for (int i=0; i<inputs; i++)
    SPAWN_METHOD(grantChanged[i], BasicArbiter::updateGrant, i, false);

  // Method for each select port, updating the signal when appropriate.
  for (int i=0; i<outputs; i++)
    SPAWN_METHOD(selectionChanged[i], BasicArbiter::updateSelect, i, false);

}

BasicArbiter::~BasicArbiter() {
  delete arbiter;
  arbiter = NULL;
}
