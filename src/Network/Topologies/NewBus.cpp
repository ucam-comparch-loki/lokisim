/*
 * NewBus.cpp
 *
 *  Created on: 8 Sep 2011
 *      Author: db434
 */

#include "NewBus.h"

int NewBus::numOutputs() const {
  return outputs;
}

void NewBus::receivedData() {
  // Just copy the values straight through.
  dataOut.write(dataIn.read());
  computeSwitching();
}

void NewBus::receivedAck() {
  dataIn.ack();
}

void NewBus::computeSwitching() {
  unsigned long cycle = Instrumentation::currentCycle();
  assert(lastWriteTime != cycle);
  lastWriteTime = cycle;

  DataType newData = dataIn.read();
  unsigned int dataDiff = newData.payload().toInt() ^ lastData.payload().toInt();
  unsigned int channelDiff = newData.channelID().getData() ^ lastData.channelID().getData();

  int bitsSwitched = __builtin_popcount(dataDiff) + __builtin_popcount(channelDiff);

  // Is the distance that the switching occurred over:
  //  1. The width of the network (length of the bus), or
  //  2. Width + height?
  Instrumentation::networkActivity(id, 0, 0, size.first, bitsSwitched);

  lastData = newData;
}

NewBus::NewBus(const sc_module_name& name, const ComponentID& ID, int numOutputPorts,
               Dimension size) :
    Component(name, ID),
    outputs(numOutputPorts),
    size(size) {

  lastData = DataType();
  lastWriteTime = -1;

  SC_METHOD(receivedData);
  sensitive << dataIn;
  dont_initialize();

  SC_METHOD(receivedAck);
  sensitive << dataOut.ack_finder();
  dont_initialize();

}
