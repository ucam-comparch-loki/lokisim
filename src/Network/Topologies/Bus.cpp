/*
 * Bus.cpp
 *
 *  Created on: 8 Mar 2011
 *      Author: db434
 */

#include "Bus.h"
#include "../../Datatype/AddressedWord.h"

void Bus::receivedData() {
  assert(readyOut.read());

  AddressedWord data = dataIn.read();
  std::vector<uint8_t> destinations;

  // Find out which outputs to send this data on.
  getDestinations(data.channelID(), destinations);

  for(unsigned int i=0; i<destinations.size(); i++) {
    const int output = destinations[i];

    // If multicasting, may need to change the channel ID for each output.
    dataOut[output].write(data);
    newData[output].write(true);
    outstandingCredits.push_back(output);
  }

  readyOut.write(false);
}

void Bus::receivedCredit() {
  std::list<uint8_t>::iterator iter = outstandingCredits.begin();

  while(iter != outstandingCredits.end()) {
    // If a recipient has deasserted the "newData" signal, remove them from
    // the list of recipients we are waiting for.
    if(!newData[*iter].read()) outstandingCredits.erase(iter);
    iter++;
  }

  if(outstandingCredits.empty()) {
    readyOut.write(true);
  }
}

void Bus::getDestinations(ChannelID address, std::vector<uint8_t>& outputs) const {
  bool multicast = false;
  if(multicast) {
    // Figure out which destinations are represented by the address.
  }
  else {
    outputs.push_back((address - startAddress)/channelsPerOutput);
  }
}

Bus::Bus(sc_module_name name, ComponentID ID, int numOutputs,
         int channelsPerOutput, ChannelID startAddr) :
    Component(name, ID) {
  dataOut = new DataOutput[numOutputs];
  newData = new sc_inout<bool>[numOutputs];

  this->numOutputs = numOutputs;
  this->channelsPerOutput = channelsPerOutput;
  this->startAddress = startAddr;
}

Bus::~Bus() {
  delete[] dataOut;
  delete[] newData;
}
