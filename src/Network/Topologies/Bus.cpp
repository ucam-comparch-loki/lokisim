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
    receivedData();

    // Pulse an acknowledgement.
    ackDataIn.write(true);
    wait(sc_core::SC_ZERO_TIME);
    ackDataIn.write(false);
  }
}

void Bus::receivedData() {
  DataType data = dataIn.read();

  const PortIndex output = getDestination(data.channelID());
  assert(output < numOutputs);

  // If multicasting, may need to change the channel ID for each output.
  dataOut[output].write(data);
  validDataOut[output].write(true);

  // Wait until receipt of the data is acknowledged.
  wait(ackDataOut[output].default_event());
  receivedCredit(output);
}

void Bus::receivedCredit(PortIndex output) {
  validDataOut[output].write(false);
}

PortIndex Bus::getDestination(const ChannelID& address) const {
  // TODO: allow non-local communication
  //TODO: Check again - unsure whether this is doing what is intended
  return (address.getGlobalChannelNumber(OUTPUT_CHANNELS_PER_TILE, CORE_OUTPUT_CHANNELS) - startAddress.getGlobalChannelNumber(OUTPUT_CHANNELS_PER_TILE, CORE_OUTPUT_CHANNELS))/channelsPerOutput;
}

void Bus::computeSwitching() {
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

Bus::Bus(sc_module_name name, const ComponentID& ID, int numOutputPorts,
         int numOutputChannels, const ChannelID& startAddr, Dimension size) :
    Component(name, ID) {

  dataOut      = new DataOutput[numOutputPorts];
  validDataOut = new ReadyOutput[numOutputPorts];
  ackDataOut   = new ReadyInput[numOutputPorts];

  this->numOutputs = numOutputPorts;
  this->channelsPerOutput = numOutputChannels/numOutputPorts;
  this->startAddress = startAddr;
  this->size = size;

  SC_THREAD(mainLoop);

  SC_METHOD(computeSwitching);
  sensitive << dataIn;
  dont_initialize();
}

Bus::~Bus() {
  delete[] dataOut;
  delete[] validDataOut;
  delete[] ackDataOut;
}
