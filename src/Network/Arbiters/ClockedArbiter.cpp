/*
 * BasicArbiter.cpp
 *
 *  Created on: 6 Sep 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "ClockedArbiter.h"
#include "../../Arbitration/ArbiterBase.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/Network.h"

using sc_core::sc_module_name;

int ClockedArbiter::numInputs()  const {return iRequest.length();}
int ClockedArbiter::numOutputs() const {return oSelect.length();}

const sc_event& ClockedArbiter::canGrantNow(int output, const ChannelIndex destination) {
  ChannelIndex channel;

  // Determine which "ready" signal to check. Memories only have one, but cores
  // have one for each buffer.
  if (numOutputs() == 1)
    channel = 0;
  else
    channel = destination;

  // If the destination is ready to receive data, we can send the grant
  // immediately.
  if (iReady[channel].read()) {
    grantEvent.notify(sc_core::SC_ZERO_TIME);
    return grantEvent;
  }
  // Otherwise, we must wait until the destination is ready.
  else {
    return iReady[channel].posedge_event();
  }
}

const sc_event& ClockedArbiter::stallGrant(int output) {
  // We must stop granting requests if the destination component stops being
  // ready to receive more data.

  // The channel we are aiming to reach through this output.
  ChannelIndex target;

  if (numOutputs() == 1)
    target = 0;
  else {
    MuxSelect input = selections[output];
    loki_assert(input != NO_SELECTION);

    target = iRequest[input].read();
    loki_assert(target != NO_REQUEST);
  }

  return iReady[target].negedge_event();
}

void ClockedArbiter::arbitrate(int output) {

  switch (state[output]) {
    // We had no requests, but now that this method has been called again, we
    // should have at least one. Wait until the clock edge to arbitrate.
    case NO_REQUESTS: {
      if (haveRequest()) {
        state[output] = HAVE_REQUESTS;
        next_trigger(sc_core::SC_ZERO_TIME);
        break;
      }
      else {
//        cout << this->name() << " waiting for requests" << endl;
        changeSelection(output, NO_SELECTION);
        next_trigger(receivedRequest);
        break;
      }
    }

    // We have requests, and it is time to perform the arbitration.
    case HAVE_REQUESTS: {
//      cout << this->name() << " arbitrating" << endl;
      MuxSelect grant = arbiter->getGrant();
      ChannelIndex destination = (grant == NO_SELECTION) ? -1 : iRequest[grant].read();

      // Check whether any other outputs are already sending to this channel.
      // TODO: have more restrictions so we can't get into this situation.
      // e.g. separate instruction and data crossbars.
      bool inUse = false;
      if (destination != -1) {
        for (int i=0; i<numOutputs(); i++) {
          if (i != output && destinations[i] == destination) {
            inUse = true;
            break;
          }
        }
      }

      // Reject the grant if the destination is already in use.
      if (inUse) {
        grant = NO_SELECTION;
        destination = -1;
      }

      selections[output] = grant;
      destinations[output] = destination;

      if (ENERGY_TRACE)
        Instrumentation::Network::arbitration();

      if (grant != ArbiterBase::NO_GRANT) {    // Successful grant
        loki_assert_with_message(grant < numInputs(), "Grant = %d", grant);

//        cout << this->name() << " selected " << (int)grant << "; waiting for flow control" << endl;

        // Remove the granted request from consideration.
        requestVec[grant] = false;

        // Update the internal array so that no one else issues the same grant,
        // but don't change the signal until we are sure that it is possible to
        // send the data.
        grantVec[grant] = true;

        state[output] = WAITING_TO_GRANT;

        next_trigger(canGrantNow(output, destination));
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
      MuxSelect granted = selections[output];
      loki_assert_with_message(granted < numInputs(), "Granted = %d", granted);
      loki_assert(granted >= 0);

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
      MuxSelect granted = selections[output];
      loki_assert_with_message(granted < numInputs(), "Granted = %d", granted);
      loki_assert(granted != NO_SELECTION);

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

void ClockedArbiter::grant(int input, int output) {
  grantVec[input] = true;
  grantChanged[input].notify();

  changeSelection(output, input);
}

void ClockedArbiter::deassertGrant(int input, int output) {
  grantVec[input] = false;
  grantChanged[input].notify();

  changeSelection(output, NO_SELECTION);
}

void ClockedArbiter::changeSelection(int output, MuxSelect value) {
  selections[output] = value;

  if (value == NO_SELECTION)
    destinations[output] = -1;
  else
    destinations[output] = iRequest[value].read();

  selectionChanged[output].notify();
}

bool ClockedArbiter::haveRequest() const {
  // A simple loop through the vector for now. If this proves to be too
  // expensive, could keep a separate count.
  for (int i=0; i<numInputs(); i++) {
    if (requestVec[i] && !grantVec[i]) return true;
  }

  // No requests were found in the vector.
  return false;
}

void ClockedArbiter::requestChanged(int input) {
  if (iRequest[input].read() == NO_REQUEST) {
    requestVec[input] = false;
  }
  else {
    PortIndex outputWanted = outputToUse(input);

    if (iReady[outputWanted].read()) {
      requestVec[input] = true;
      receivedRequest.notify();
    }
    else {
      requestVec[input] = false;
    }

    next_trigger(iReady[outputWanted].default_event() | iRequest[input].default_event());
  }
}

PortIndex ClockedArbiter::outputToUse(PortIndex input) {
  // Memories only have one buffer and one ready signal which cover all channels.
  // Cores have separate buffers and separate ready signals.

  if (numOutputs() == 1)
    return 0;
  else
    return iRequest[input].read();
}

void ClockedArbiter::updateGrant(int input) {
  // Update the output signal to the value held in the internal vector.
  oGrant[input].write(grantVec[input]);
}

void ClockedArbiter::updateSelect(int output) {
  // Wait until the start of the next cycle before changing the select signal.
  // Arbitration is done the cycle before data is sent.
  if (clock.posedge()) {  // The clock edge
    oSelect[output].write(selections[output]);
//    cout << this->name() << " granted input " << (int)(selectVec[output]) << endl;
    next_trigger(selectionChanged[output]);
  }
  else {                  // Select signal changed
    next_trigger(clock.posedge_event());
  }
}

void ClockedArbiter::reportStalls(ostream& os) {
  for (uint i=0; i<iRequest.length(); i++) {
    if (requestVec[i]) {
      os << this->name() << " still has active request on input " << i << endl;
    }
  }
}

ClockedArbiter::ClockedArbiter(const sc_module_name& name, ComponentID ID,
                           int inputs, int outputs, bool wormhole, int flowControlSignals) :
    LokiComponent(name, ID),
    wormhole(wormhole),
    requestVec(inputs, false),
    grantVec(inputs, false),
    selections(outputs, -1),
    destinations(outputs, -1),
    state(outputs, NO_REQUESTS) {

  loki_assert(inputs > 0);
  loki_assert(outputs > 0);

  Instrumentation::Network::arbiterCreated();

  iRequest.init(inputs);
  oGrant.init(inputs);
  oSelect.init(outputs);
  iReady.init(flowControlSignals);

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
    SPAWN_METHOD(receivedRequest, ClockedArbiter::arbitrate, i, false);

  // Generate a method to watch each request port, taking appropriate action
  // whenever the signal changes.
  for (int i=0; i<inputs; i++)
    SPAWN_METHOD(iRequest[i], ClockedArbiter::requestChanged, i, false);

  // Method for each grant port, updating the grant signal when appropriate.
  for (int i=0; i<inputs; i++)
    SPAWN_METHOD(grantChanged[i], ClockedArbiter::updateGrant, i, false);

  // Method for each select port, updating the signal when appropriate.
  for (int i=0; i<outputs; i++)
    SPAWN_METHOD(selectionChanged[i], ClockedArbiter::updateSelect, i, false);

}

ClockedArbiter::~ClockedArbiter() {
  delete arbiter;
  arbiter = NULL;
}
