/*
 * ArbiterComponent.cpp
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#include "ArbiterComponent.h"
#include "../../Arbitration/RoundRobinArbiter.h"
#include "../../Datatype/AddressedWord.h"

void ArbiterComponent::mainLoop() {
  while(true) {
    // Wait for something interesting to happen.
    wait(clock.negedge_event() | ack);

    if(clock.negedge()) arbitrate();
    else {
      // Pull down the valid signal for any outputs which have sent acknowledgements.
      for(int i=0; i<numOutputs; i++) {
        if(ackDataOut[i].read()) validDataOut[i].write(false);
      }
    }
  }
}

void ArbiterComponent::arbitrate() {

  if(!haveData) return;

  RequestList requests;
  GrantList grants;

  for(int i=0; i<numInputs; i++) {
    if(validDataIn[i].read()) {
      Request request(i, dataIn[i].read().endOfPacket());
      requests.push_back(request);
    }
  }

  if(requests.empty()) {
    haveData = false;
    return;
  }

  arbiter->arbitrate(requests, grants);

  for(unsigned int i=0; i<grants.size(); i++) {
    const int input = grants[i].first;
    const int output = grants[i].second;

    // FIXME: a request may be granted, but then blocked by flow control.
    // Another, later request may still be allowed to send. Seems unfair.
//    if(!validDataOut[output].read()) {
      dataOut[output].write(dataIn[input].read());
      validDataOut[output].write(true);
      ackDataIn[input].write(!ackDataIn[input].read()); // Toggle value
//    }
  }

}

void ArbiterComponent::ackArrived() {
  ack.notify();
}

void ArbiterComponent::dataArrived() {
  haveData = true;
}

ArbiterComponent::ArbiterComponent(sc_module_name name, ComponentID ID,
                                   int inputs, int outputs) :
    Component(name, ID) {

  numInputs    = inputs;
  numOutputs   = outputs;

  dataIn       = new sc_in<AddressedWord>[inputs];
  validDataIn  = new sc_in<bool>[inputs];
  ackDataIn    = new sc_out<bool>[inputs];

  dataOut      = new sc_out<AddressedWord>[outputs];
  validDataOut = new sc_out<bool>[outputs];
  ackDataOut   = new sc_in<bool>[outputs];

  arbiter      = new RoundRobinArbiter2(inputs, outputs, false);

  SC_THREAD(mainLoop);

  SC_METHOD(dataArrived);
  for(int i=0; i<inputs; i++) sensitive << validDataIn[i].pos();
  dont_initialize();

  SC_METHOD(ackArrived);
  for(int i=0; i<outputs; i++) sensitive << ackDataOut[i].pos();
  dont_initialize();

  end_module();
}

ArbiterComponent::~ArbiterComponent() {
  delete[] dataIn;
  delete[] validDataIn;
  delete[] ackDataIn;

  delete[] dataOut;
  delete[] validDataOut;
  delete[] ackDataOut;

  delete arbiter;
}
