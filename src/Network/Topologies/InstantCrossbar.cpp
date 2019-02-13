/*
 * InstantCrossbar.cpp
 *
 *  Created on: 27 Feb 2014
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "InstantCrossbar.h"
#include "../../Exceptions/InvalidOptionException.h"
#include "../../Utility/Assert.h"

using sc_core::sc_module_name;

PortIndex InstantCrossbar::getDestination(ChannelID address) const {
  // This network is deprecated and is currently only used in one place.
  loki_assert(level == NONE);
  return 0;
}

void InstantCrossbar::mainLoop(PortIndex port) {
  NetworkData data = iData[port].read();
  PortIndex output = getDestination(data.channelID());

  loki_assert_with_message(output < oData.size(),
      "Outputs = %d, requested port = %d", oData.size(), output);

  switch (state[port]) {
    // New data arrived -> issue request.
    case IDLE:
//      cout << this->name() << " issuing request for " << data << endl;
      requests[port][output].write(0); // Access first channel through output port
      state[port] = ARBITRATING;
      next_trigger(grants[port][output].default_event());
      break;

    // Request granted -> wait until data has made it through.
    case ARBITRATING:
      state[port] = FINISHED;
      next_trigger(oData[output].ack_event());
      break;

    // Finished with data -> wait for next data.
    case FINISHED:
      requests[port][output].write(NO_REQUEST);
      state[port] = IDLE;
      next_trigger(iData[port].default_event()); // TODO: remove?
      break;

    default:
      throw InvalidOptionException("crossbar state", state[port]);
      break;
  }
}

InstantCrossbar::InstantCrossbar(const sc_module_name& name,
                                 int inputs,
                                 int outputs,
                                 int outputsPerComponent,
                                 HierarchyLevel level,
                                 int buffersPerComponent) :
    Network(name, inputs, outputs, level),
    iData("iData", inputs),
    oData("oData", outputs),
    crossbar("internal", inputs, outputs, outputsPerComponent, level, buffersPerComponent),
    state(inputs, IDLE) {

  // Create ports and signals.
  requests.init("request", crossbar.iRequest);
  grants.init("grant", crossbar.oGrant);
  iReady.init("iReady", crossbar.iReady);

  // Connect up the inner crossbar.
  crossbar.clock(clock);
  crossbar.iData(iData);
  crossbar.oData(oData);
  crossbar.iRequest(requests);
  crossbar.oGrant(grants);
  crossbar.iReady(iReady);

  for (uint i=0; i<requests.size(); i++)
    for (uint j=0; j<requests[i].size(); j++)
      requests[i][j].write(NO_REQUEST);

  // Create a method for each data input port.
  for (uint i=0; i<iData.size(); i++)
    SPAWN_METHOD(iData[i], InstantCrossbar::mainLoop, i, false);

}

