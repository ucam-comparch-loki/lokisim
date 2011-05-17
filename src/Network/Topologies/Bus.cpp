/*
 * Bus.cpp
 *
 *  Created on: 31 Mar 2011
 *      Author: db434
 */

#include "Bus.h"

void Bus::mainLoop() {
  while(true) {
    wait(clock.posedge_event());

    ackDataIn[0].write(false);
    wait(sc_core::SC_ZERO_TIME);  // Allow time for the valid signal to be deasserted.
    if(!validDataIn[0].read()) continue;

    receivedData();

    // Pulse an acknowledgement.
    ackDataIn[0].write(true);
  }
}

void Bus::receivedData() {
  DataType data = dataIn[0].read();

  const PortIndex output = getDestination(data.channelID());
  assert(output < numOutputs);

  dataOut[output].write(data);
  validDataOut[output].write(true);

  // Wait until receipt of the data is acknowledged.
  wait(ackDataOut[output].default_event());
  receivedCredit(output);
}

void Bus::receivedCredit(PortIndex output) {
  validDataOut[output].write(false);
}

void Bus::computeSwitching() {
  DataType newData = dataIn[0].read();
  unsigned int dataDiff = newData.payload().toInt() ^ lastData.payload().toInt();
  unsigned int channelDiff = newData.channelID().getData() ^ lastData.channelID().getData();

  int bitsSwitched = __builtin_popcount(dataDiff) + __builtin_popcount(channelDiff);

  // Is the distance that the switching occurred over:
  //  1. The width of the network (length of the bus), or
  //  2. Width + height?
  Instrumentation::networkActivity(id, 0, 0, size.first, bitsSwitched);

  lastData = newData;
}

Bus::Bus(sc_module_name name, const ComponentID& ID, int numOutputPorts,
         HierarchyLevel level, Dimension size) :
    Network(name, ID, 1, numOutputPorts, level, size) {

  SC_THREAD(mainLoop);

  SC_METHOD(computeSwitching);
  sensitive << dataIn[0];
  dont_initialize();
}
