/*
 * Bus.cpp
 *
 *  Created on: 8 Mar 2011
 *      Author: db434
 */

#include "Bus.h"
#include "../../Datatype/AddressedWord.h"

void Bus::mainLoop() {
  while(true) {
    wait(dataIn.default_event());
    readyOut.write(false);
    receivedData();
    while(!outstandingCredits.empty()) {
      wait(credit);
      receivedCredit();
    }

    readyOut.write(true);
  }
}

void Bus::receivedData() {
  assert(outstandingCredits.empty());

  AddressedWord data = dataIn.read();
  std::vector<PortIndex> destinations;

  // Find out which outputs to send this data on.
  getDestinations(data.channelID(), destinations);

  for(unsigned int i=0; i<destinations.size(); i++) {
    const int output = destinations[i];
    assert(output < numOutputs);

    // If multicasting, may need to change the channel ID for each output.
    dataOut[output].write(data);
    newData[output].write(true);
    outstandingCredits.push_back(output);
  }
}

void Bus::receivedCredit() {
  std::list<PortIndex>::iterator iter = outstandingCredits.begin();
  int size = outstandingCredits.size();

  for(int i=0; i < size; i++, iter++) {
    // If a recipient has consumed the given data, remove them from the list of
    // recipients we are waiting for, and deassert the newData signal.
    if(dataRead[*iter].event()) {
      outstandingCredits.erase(iter);
      newData[*iter].write(false);
      iter--;
    }
  }
}

void Bus::getDestinations(ChannelID address, std::vector<PortIndex>& outputs) const {
  bool multicast = false;
  if(multicast) {
    // Figure out which destinations are represented by the address.
  }
  else {
    outputs.push_back((address - startAddress)/channelsPerOutput);
  }
}

void Bus::creditArrived() {
  credit.notify();
}

Bus::Bus(sc_module_name name, ComponentID ID, int numOutputs,
         int channelsPerOutput, ChannelID startAddr) :
    Component(name, ID) {
  dataOut = new DataOutput[numOutputs];
  newData = new ReadyOutput[numOutputs];
  dataRead = new ReadyInput[numOutputs];

  this->numOutputs = numOutputs;
  this->channelsPerOutput = channelsPerOutput;
  this->startAddress = startAddr;

  SC_THREAD(mainLoop);

  SC_METHOD(creditArrived);
  for(int i=0; i<numOutputs; i++) sensitive << dataRead[i];
  dont_initialize();

  readyOut.initialize(true);

  end_module();
}

Bus::~Bus() {
  delete[] dataOut;
  delete[] newData;
  delete[] dataRead;
}
