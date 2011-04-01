/*
 * MulticastBus.cpp
 *
 *  Created on: 8 Mar 2011
 *      Author: db434
 */

#include "MulticastBus.h"
#include "../../Datatype/AddressedWord.h"

void MulticastBus::mainLoop() {
  while(true) {
    wait(dataIn.default_event());
    readyOut.write(false);
    receivedData();
    while(!outstandingCredits.empty()) {
      wait(credit);
      checkCredits();
    }
    readyOut.write(true);
  }
}

void MulticastBus::receivedData() {
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

void MulticastBus::checkCredits() {
  std::list<PortIndex>::iterator iter = outstandingCredits.begin();
  int size = outstandingCredits.size();

  for(int i=0; i < size; i++, iter++) {
    // If a recipient has consumed the given data, remove them from the list of
    // recipients we are waiting for, and deassert the newData signal.
    if(dataRead[*iter].event()) {
      receivedCredit(*iter);
      outstandingCredits.erase(iter);
      iter--;
    }
  }
}

void MulticastBus::getDestinations(ChannelID address, std::vector<PortIndex>& outputs) const {
  // Would it be sensible to enforce that MulticastBuses only ever receive
  // multicast addresses? Might make things simpler.
  bool multicast = false;
  if(multicast) {
    // Figure out which destinations are represented by the address.
  }
  else {
    outputs.push_back((address - startAddress)/channelsPerOutput);
  }
}

void MulticastBus::creditArrived() {
  credit.notify();
}

MulticastBus::MulticastBus(sc_module_name name, ComponentID ID, int numOutputs,
                           int channelsPerOutput, ChannelID startAddr, Dimension size) :
    Bus(name, ID, numOutputs, channelsPerOutput, startAddr, size) {

  SC_METHOD(creditArrived);
  for(int i=0; i<numOutputs; i++) sensitive << dataRead[i];
  dont_initialize();

  end_module();
}
