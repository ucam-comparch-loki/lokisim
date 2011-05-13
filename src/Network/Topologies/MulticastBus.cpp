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
    // Wait for data to arrive.
    wait(validDataIn.posedge_event());
    receivedData();

    // Wait for all credits to arrive.
    while(!outstandingCredits.empty()) {
      wait(credit);
      checkCredits();
    }

    // Send a credit back to the sender.
    creditsOut.write(AddressedWord(1, creditDestination));

    validCreditOut.write(true);
    wait(ackCreditOut.posedge_event());
    validCreditOut.write(false);

    // Pulse an acknowledgement. Do this very late so the component doesn't
    // send more data before the credits have been received.
    ackDataIn.write(true);
    wait(sc_core::SC_ZERO_TIME);
    ackDataIn.write(false);
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
    if(ackDataOut[*iter].event()) {
      validDataOut[*iter].write(false);
    }
    // If a recipient has consumed the given data, remove them from the list of
    // recipients we are waiting for, and deassert the newData signal.
    if(validCreditIn[*iter].read()) {
      receivedCredit(*iter);
      creditDestination = creditsIn[*iter].read().channelID();
      outstandingCredits.erase(iter);
      iter--;
    }
  }
}

void MulticastBus::receivedCredit(PortIndex output) {
  validDataOut[output].write(false);
  ackCreditIn[output].write(true);
  wait(sc_core::SC_ZERO_TIME);
  ackCreditIn[output].write(false);
}

void MulticastBus::getDestinations(ChannelID address, std::vector<PortIndex>& outputs) const {
  // Would it be sensible to enforce that MulticastBuses only ever receive
  // multicast addresses? Might make things simpler.
  // But that would make it difficult to deal with non-local traffic.
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

  creditsIn     = new CreditInput[numOutputs];
  validCreditIn = new ReadyInput[numOutputs];
  ackCreditIn   = new ReadyOutput[numOutputs];

  SC_METHOD(creditArrived);
  for(int i=0; i<numOutputs; i++) sensitive << validCreditIn[i].pos() << ackDataOut[i];
  dont_initialize();
}

MulticastBus::~MulticastBus() {
  delete[] creditsIn;
  delete[] validCreditIn;
  delete[] ackCreditIn;
}
