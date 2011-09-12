/*
 * NewBus.cpp
 *
 *  Created on: 8 Sep 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "NewBus.h"

int NewBus::numOutputs() const {
  return outputs;
}

void NewBus::receivedData() {
  // Just copy the values straight through.
  validDataOut.write(validDataIn.read());

  // Only perform the expensive write if the data has changed.
  if(validDataIn.read()) dataOut.write(dataIn.read());
}

void NewBus::receivedAck(int output) {
  // Since this is a simple single-destination bus, we know that any received
  // acknowledgement is the final one.
  if(ackDataOut[output].read()) receivedAllAcks.notify();
  else                          allAcksDeasserted.notify();
}

void NewBus::sendAck() {
  if(ackDataIn.read()) {
    ackDataIn.write(false);
    next_trigger(receivedAllAcks);
  }
  else {
    ackDataIn.write(true);
    next_trigger(allAcksDeasserted);
  }
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

  ackDataOut = new ReadyInput[numOutputPorts];

  lastData = DataType();
  lastWriteTime = -1;

  SC_METHOD(receivedData);
  sensitive << validDataIn;
  dont_initialize();

  SC_METHOD(computeSwitching);
  sensitive << dataIn;
  dont_initialize();

  SC_METHOD(sendAck);
  sensitive << receivedAllAcks;
  dont_initialize();

  // Generate a method for each acknowledgement port, to keep track of how many
  // acks we are waiting for at any given time.
  for(int i=0; i<outputs; i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread
    options.dont_initialize();
    options.set_sensitivity(&(ackDataOut[i].value_changed())); // Sensitive to this event

    // Create the method.
    sc_spawn(sc_bind(&NewBus::receivedAck, this, i), 0, &options);
  }

}

NewBus::~NewBus() {
  delete[] ackDataOut;
}
