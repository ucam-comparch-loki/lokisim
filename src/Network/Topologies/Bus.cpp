/*
 * Bus.cpp
 *
 *  Created on: 31 Mar 2011
 *      Author: db434
 */

#include "Bus.h"

void Bus::mainLoop() {
  while(true) {
    wait(dataIn.default_event());
    canReceiveData.write(false);
    receivedData();
    canReceiveData.write(true);
  }
}

void Bus::receivedData() {
  DataType data = dataIn.read();

  const PortIndex output = getDestination(data.channelID());
  assert(output < numOutputs);

  // If multicasting, may need to change the channel ID for each output.
  dataOut[output].write(data);
  validOut[output].write(true);

  // Wait until receipt of the data is acknowledged.
  wait(ackIn[output].default_event());
  receivedCredit(output);
}

void Bus::receivedCredit(PortIndex output) {
  validOut[output].write(false);
}

PortIndex Bus::getDestination(ChannelID address) const {
  // TODO: allow non-local communication
  return (address - startAddress)/channelsPerOutput;
}

void Bus::computeSwitching() {
  DataType newData = dataIn.read();
  unsigned int dataDiff = newData.payload().toInt() ^ lastData.payload().toInt();
  unsigned int channelDiff = newData.channelID() ^ lastData.channelID();

  int bitsSwitched = __builtin_popcount(dataDiff) + __builtin_popcount(channelDiff);

  // Is the distance that the switching occurred over:
  //  1. The width of the network (length of the bus), or
  //  2. Width + height?
  Instrumentation::networkActivity(id, 0, 0, size.first, bitsSwitched);

  lastData = newData;
}

Bus::Bus(sc_module_name name, ComponentID ID, int numOutputPorts,
         int numOutputChannels, ChannelID startAddr, Dimension size) :
    Component(name, ID) {

  dataOut  = new DataOutput[numOutputPorts];
  validOut  = new ReadyOutput[numOutputPorts];
  ackIn = new ReadyInput[numOutputPorts];

  this->numOutputs = numOutputPorts;
  this->channelsPerOutput = numOutputChannels/numOutputPorts;
  this->startAddress = startAddr;
  this->size = size;

  SC_THREAD(mainLoop);

  SC_METHOD(computeSwitching);
  sensitive << dataIn;
  dont_initialize();

  canReceiveData.initialize(true);

  // If this is here, we can't use MulticastBus.
  // If it isn't here, we can't use Bus.
  end_module();
}

Bus::~Bus() {
  delete[] dataOut;
  delete[] validOut;
  delete[] ackIn;
}
