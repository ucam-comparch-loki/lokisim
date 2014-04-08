/*
 * InstantCrossbar.cpp
 *
 *  Created on: 27 Feb 2014
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "InstantCrossbar.h"

void InstantCrossbar::mainLoop(PortIndex port) {
  DataType data = dataIn[port].read();
  PortIndex output = getDestination(data.channelID());
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
      next_trigger(dataOut[output].ack_event());
      break;

    // Finished with data -> wait for next data.
    case FINISHED:
      requests[port][output].write(NO_REQUEST);
      state[port] = IDLE;
      next_trigger(dataIn[port].default_event()); // TODO: remove?
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
  readyIn.init(arbiters, buffersPerComponent);
  requests.init(numInputPorts(), arbiters);
  grants.init(numInputPorts(), arbiters);

  // Connect up the inner crossbar.
  crossbar.clock(clock);
  for (uint i=0; i<dataIn.length(); i++)
    crossbar.dataIn[i](dataIn[i]);
  for (uint i=0; i<dataOut.length(); i++)
    crossbar.dataOut[i](dataOut[i]);

  for (uint i=0; i<numInputPorts(); i++) {
    for (uint j=0; j<arbiters; j++) {
      crossbar.requestsIn[i][j](requests[i][j]);
      crossbar.grantsOut[i][j](grants[i][j]);
      requests[i][j].write(NO_REQUEST);
    }
  }

  for (uint i=0; i<arbiters; i++)
    for (int j=0; j<buffersPerComponent; j++)
      crossbar.readyIn[i][j](readyIn[i][j]);

  // Create a method for each data input port.
  for (uint i=0; i<numInputPorts(); i++)
    SPAWN_METHOD(dataIn[i], InstantCrossbar::mainLoop, i, false);

}

InstantCrossbar::~InstantCrossbar() {
  // Do nothing
}

