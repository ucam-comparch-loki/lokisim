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
    wait(clock.posedge_event());
    ackDataIn.write(false);
    if(!validDataIn.read()) continue;

    receivedData();
    while(!outstandingCredits.empty()) {
      wait(credit);
      checkCredits();
    }
    ackDataIn.write(true);

    if(!canSendCredits.read()) wait(canSendCredits.posedge_event());
    creditsOut.write(AddressedWord(1, creditDestination));
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
    validDataOut[output].write(true);
    outstandingCredits.push_back(output);
  }
}

void MulticastBus::checkCredits() {
  std::list<PortIndex>::iterator iter = outstandingCredits.begin();
  int size = outstandingCredits.size();

  for(int i=0; i < size; i++, iter++) {
    // If a recipient has consumed the given data, remove them from the list of
    // recipients we are waiting for, and deassert the newData signal.
    if(creditsIn[*iter].event()) {
      receivedCredit(*iter);
      creditDestination = creditsIn[*iter].read().channelID();
      outstandingCredits.erase(iter);
      iter--;
    }
  }
}

void MulticastBus::getDestinations(const ChannelID& address, std::vector<PortIndex>& outputs) const {
  // Would it be sensible to enforce that MulticastBuses only ever receive
  // multicast addresses? Might make things simpler.
  // But that would make it difficult to deal with non-local traffic.
  bool multicast = false;
  if(multicast) {
    // Figure out which destinations are represented by the address.
  }
  else {
    //TODO: Check again - unsure whether this is doing what is intended
    outputs.push_back((address.getGlobalChannelNumber(OUTPUT_CHANNELS_PER_TILE, CORE_OUTPUT_CHANNELS) - startAddress.getGlobalChannelNumber(OUTPUT_CHANNELS_PER_TILE, CORE_OUTPUT_CHANNELS))/channelsPerOutput);
  }
}

void MulticastBus::creditArrived() {
  credit.notify();
}

MulticastBus::MulticastBus(sc_module_name name, const ComponentID& ID, int numOutputs,
                           int channelsPerOutput, const ChannelID& startAddr, Dimension size) :
    Bus(name, ID, numOutputs, channelsPerOutput, startAddr, size) {

  creditsIn         = new CreditInput[numOutputs];
  canReceiveCredits = new ReadyOutput[numOutputs];

  for(int i=0; i<numOutputs; i++) canReceiveCredits[i].initialize(true);

  SC_METHOD(creditArrived);
  for(int i=0; i<numOutputs; i++) sensitive << creditsIn[i];
  dont_initialize();
}

MulticastBus::~MulticastBus() {
  delete[] creditsIn;
  delete[] canReceiveCredits;
}
