/*
 * OutputBuffers.cpp
 *
 *  Created on: 2 Mar 2011
 *      Author: db434
 */

#include "OutputBuffers.h"
#include "../Arbitration/RoundRobinArbiter.h"
#include "../Datatype/AddressedWord.h"

void OutputBuffers::sendAll() {
  while(true) {
    wait(clock.posedge_event());

    // Possible optimisation: keep track of whether any of the buffers are
    // non-empty, so we can skip all this if we know there is nothing to do.

    RequestList& requests = allRequests();
    GrantList grants;
    arbiter->arbitrate(requests, grants);
    delete &requests;

    if(grants.size() == 0) return;

    for(uint i=0; i<grants.size(); i++) {
      doRead[grants[i].first].write(true);
    }

    // Wait until the buffers start returning data.
    wait(dataFromBuffers[grants[0].first].default_event());

    for(uint i=0; i<grants.size(); i++) {
      sendSingle(grants[i].first, grants[i].second);
    }
  }
}

void OutputBuffers::sendSingle(int bufferIndex, int output) {
  dataOut[output].write(dataFromBuffers[bufferIndex].read());
}

bool OutputBuffers::canSend(int bufferIndex) const {
  return !buffers[bufferIndex]->canSend();
}

void OutputBuffers::receivedCredits() {
  for(uint i=0; i<numOutputs; i++) {
    if(creditsIn[i].event()) {
      // Determine which buffer the credit is for.
      ChannelIndex destination = creditsIn[i].read().channelID() % numInputs;
      creditSig[destination].write(true);
    }
  }
}

RequestList& OutputBuffers::allRequests() const {
  RequestList* requests = new RequestList();

  for(uint i=0; i<buffers.size(); i++) {
    if(canSend(i)) {
      requests->push_back(Request(i, buffers[i]->peek().endOfPacket()));
    }
  }

  return *requests;
}

OutputBuffers::OutputBuffers(sc_module_name name, int inputs, int outputs, int buffSize) :
    Component(name),
    buffers(inputs) {

  numInputs  = inputs;
  numOutputs = outputs;

  // TODO: parameterise arbiter
  arbiter         = new RoundRobinArbiter2(inputs, outputs, false);

  dataIn          = new sc_in<AddressedWord>[inputs];
  creditsIn       = new sc_in<AddressedWord>[outputs];
  dataOut         = new sc_out<AddressedWord>[outputs];
  flowControlOut  = new sc_out<bool>[inputs];
  emptyBuffer     = new sc_out<bool>[inputs];

  doRead          = new sc_buffer<bool>[inputs];
  creditSig       = new sc_buffer<bool>[inputs];
  dataFromBuffers = new sc_buffer<AddressedWord>[inputs];

  for(int i=0; i<inputs; i++) {
    BufferWithCounter<AddressedWord>* buffer =
        new BufferWithCounter<AddressedWord>("buffer", i, buffSize);
    buffers.push_back(buffer);
    buffer->credits(creditSig[i]);
    buffer->dataIn(dataIn[i]);
    buffer->dataOut(dataFromBuffers[i]);
    buffer->doRead(doRead[i]);
    buffer->emptySig(emptyBuffer[i]);
    buffer->flowControlOut(flowControlOut[i]);
  }

  SC_THREAD(sendAll);

  SC_METHOD(receivedCredits);
  for(int i=0; i<outputs; i++) sensitive << creditsIn[i];
  dont_initialize();

}

OutputBuffers::~OutputBuffers() {
  delete[] dataIn;
  delete[] creditsIn;
  delete[] dataOut;
  delete[] flowControlOut;
  delete[] emptyBuffer;
  delete[] doRead;
  delete[] creditSig;
  delete[] dataFromBuffers;

  delete arbiter;

  for(uint i=0; i<buffers.size(); i++) delete buffers[i];
}
