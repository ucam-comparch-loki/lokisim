/*
 * InstantCrossbar.cpp
 *
 *  Created on: 27 Feb 2014
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "InstantCrossbar.h"

void InstantCrossbar::mainLoop(PortIndex port) {
  DataType data = iData[port].read();
  PortIndex output = getDestination(data.channelID());

  if (output >= numOutputPorts())
    cerr << this->name() << " outputs: " << numOutputPorts() << ", requested port: " << (int)output << endl;
  assert(output < numOutputPorts());

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
      assert(false);
      break;
  }
}

InstantCrossbar::InstantCrossbar(const sc_module_name& name,
                                 const ComponentID& ID,
                                 int inputs,
                                 int outputs,
                                 int outputsPerComponent,
                                 HierarchyLevel level,
                                 int buffersPerComponent) :
    Network(name, ID, inputs, outputs, level),
    crossbar("internal", ID, inputs, outputs, outputsPerComponent, level, buffersPerComponent),
    state(inputs, IDLE) {

  uint arbiters = outputs/outputsPerComponent;

  // Create ports and signals.
  iReady.init(arbiters, buffersPerComponent);
  requests.init(numInputPorts(), arbiters);
  grants.init(numInputPorts(), arbiters);

  // Connect up the inner crossbar.
  crossbar.clock(clock);
  for (uint i=0; i<iData.length(); i++)
    crossbar.iData[i](iData[i]);
  for (uint i=0; i<oData.length(); i++)
    crossbar.oData[i](oData[i]);

  for (uint i=0; i<numInputPorts(); i++) {
    for (uint j=0; j<arbiters; j++) {
      crossbar.iRequest[i][j](requests[i][j]);
      crossbar.oGrant[i][j](grants[i][j]);
      requests[i][j].write(NO_REQUEST);
    }
  }

  for (uint i=0; i<arbiters; i++)
    for (int j=0; j<buffersPerComponent; j++)
      crossbar.iReady[i][j](iReady[i][j]);

  // Create a method for each data input port.
  for (uint i=0; i<numInputPorts(); i++)
    SPAWN_METHOD(iData[i], InstantCrossbar::mainLoop, i, false);

}

InstantCrossbar::~InstantCrossbar() {
  // Do nothing
}

