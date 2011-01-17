/*
 * RoutingComponent.cpp
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#include "RoutingComponent.h"
#include "../Datatype/AddressedWord.h"
#include "Arbiters/Arbiter.h"

void RoutingComponent::newData() {
  haveData = true;
}

void RoutingComponent::sendData() {
  // Only execute the expensive loop if we know there is data to send.
  if(haveData) {
    vector<Path> requests;

    // Assuming no contention for now: just send all new data to its destination.
    for(ChannelIndex input=0; input<numInputs; input++) {

      // Check for new input data.
      if(dataIn[input].event()) {
        assert(!inputBuffers[input].full());

        inputBuffers[input].write(dataIn[input].read());
        updateReady(input);
      }

      // Try to send any data in the buffer.
      if(!inputBuffers[input].empty()) {
        // Determine which output port to send the data to.
        ChannelID destination = inputBuffers[input].peek().channelID();
        ChannelIndex outPort = computeOutput(input, destination);

        // Collect set of inputs and destinations (where destination is ready),
        // and pass them to an Arbiter, which returns the set of inputs which
        // may send.
        if(readyIn[outPort].read()) {
          Path p(input, outPort, inputBuffers[input].peek());
          requests.push_back(p);
        }
      }
    }

    // Determine which of the requests can safely be granted.
    vector<Path>& allowedToSend = arbiter_->arbitrate(requests);

    // Send all allowed requests.
    for(uint i=0; i<allowedToSend.size(); i++) {
      ChannelIndex input = allowedToSend[i].source();
      ChannelIndex output = allowedToSend[i].destination();

      AddressedWord aw = inputBuffers[input].read();
      dataOut[output].write(aw);

      Instrumentation::networkActivity(id, input, output, distance(input,output),
          bitsSwitched(input, output, aw));

      updateReady(input);
    }

    // Errors if we don't do this check.
    if(allowedToSend.size() > 0) delete &allowedToSend;
    haveData = !inputBuffers.empty();
  }
}

void RoutingComponent::updateReady(ChannelIndex input) {
  // We can accept new data if the buffer is not full.
  readyOut[input].write(!inputBuffers[input].full());
}

int RoutingComponent::bitsSwitched(ChannelIndex from, ChannelIndex to,
                                   AddressedWord data) const {
  // For now, just assume half of all bits switch for data, and the single
  // credit bit always changes. Take into account addresses too?

  switch(networkType) {
    case LOC_DATA: return 16;
    case LOC_CREDIT: return 1;
    case GLOB_DATA: return 16;
    case GLOB_CREDIT: return 1;
    default: throw std::exception();
  }
}

void RoutingComponent::printDebugMessage(ChannelIndex from, ChannelIndex to,
                                         AddressedWord data) const {

  std::string in   = "input ";
  std::string out  = "output ";
//  int inChans      = fromOutputs ? NUM_CLUSTER_OUTPUTS : NUM_CLUSTER_INPUTS;
//  int outChans     = fromOutputs ? NUM_CLUSTER_INPUTS  : NUM_CLUSTER_OUTPUTS;
//
//  cout << this->name() << " sent " << data.payload() <<
//          " from channel "<<from<<" (comp "<<from/inChans<<", "<<out<<from%inChans<<
//          ") to channel "<<to<<" (comp "<<to/outChans<<", "<<in<<to%outChans
//          << ")" << endl;
}

RoutingComponent::RoutingComponent(sc_module_name name,
                                   ComponentID ID,
                                   int numInputs,
                                   int numOutputs,
                                   int bufferSize,
                                   int type) :
    Component(name, ID),
    inputBuffers(numInputs, bufferSize, this->name()) {

  this->numInputs = numInputs;
  this->numOutputs = numOutputs;
  networkType = type;

  dataIn   = new sc_in<AddressedWord>[numInputs];
  dataOut  = new sc_out<AddressedWord>[numOutputs];
  readyIn  = new sc_in<bool>[numOutputs];
  readyOut = new sc_out<bool>[numInputs];

  SC_METHOD(newData);
  for(int i=0; i<numInputs; i++) sensitive << dataIn[i];
  dont_initialize();

  SC_METHOD(sendData);
  sensitive << clock.neg();
  dont_initialize();

  // We start off accepting any input.
  for(int i=0; i<numInputs; i++) readyOut[i].initialize(true);

  end_module();
}

RoutingComponent::~RoutingComponent() {
  delete[] dataIn;
  delete[] dataOut;
  delete[] readyOut;
  delete[] readyIn;

  delete arbiter_;
}
