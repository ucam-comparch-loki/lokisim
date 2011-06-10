/*
 * ArbiterComponent.cpp
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#include "ArbiterComponent.h"
#include "../../Arbitration/RoundRobinArbiter.h"
#include "../../Datatype/AddressedWord.h"

void ArbiterComponent::arbiterLoop() {
  switch(state) {
    case WAITING_FOR_DATA : {
      // Arbiters wait until the middle of the clock cycle to check for data
      // to allow time for it to be sent from all of the source components.
      // Source components always send data on the posedge.
      if(!clock.negedge()) {
        next_trigger(clock.negedge_event());
        return;
      }

      arbitrate();

      if(activeTransfers > 0) {
        state = WAITING_FOR_ACKS;
        next_trigger(clock.posedge_event());  // is this right?
      }
      else {cerr << "hello?" << endl;
        // There are no acknowledgements to wait for, so wait until data arrives.
        next_trigger(newDataEvent);
      }

      break;
    }

    case WAITING_FOR_ACKS : {
      checkAcks();

      state = WAITING_FOR_DATA;

      // See if there is any data waiting to be sent.
      bool haveData = false;
      for(int i=0; i<numInputs; i++) {
        if(validDataIn[i].read()) {
          haveData = true;
          break;
        }
      }

      if(activeTransfers > 0 || haveData) next_trigger(clock.negedge_event());
      else                                next_trigger(newDataEvent);

      break;
    }
  } // end switch
}

void ArbiterComponent::arbitrate() {
  // Currently implements a round-robin arbitration scheme.
  // TODO: make use of the classes in Arbitration so the behaviour can easily
  // be swapped in and out.

  int inCursor = lastAccepted;
  int outCursor = 0;

  for (int i = 0; i < numInputs; i++) {
    // Request: numInput -> endOfPacket

    inCursor++;
    if (inCursor == numInputs)
      inCursor = 0;

    if (!validDataIn[inCursor].read())
      continue;

    // Determine which output to send the data to. If this arbiter makes use
    // of wormhole routing, some outputs may be reserved.
    int outputToUse;
    if(wormhole && (reservations[inCursor] != NO_RESERVATION)) {
      outputToUse = reservations[inCursor];
    }
    else {
      // Find the next available output (skip over any which are in use
      // or reserved by wormhole-routed packets).
      while((inUse[outCursor] || (wormhole && reserved[outCursor]))
         && (outCursor < numOutputs))
        outCursor++;

      outputToUse = outCursor;
    }

    // Don't exit the loop if we have now been through all outputs. There may
    // still be reservations which can be filled.
    if(outputToUse >= numOutputs) continue;

    // Send the data, if appropriate.
    if (!inUse[outputToUse]) {
      dataOut[outputToUse].write(dataIn[inCursor].read());
      validDataOut[outputToUse].write(true);
      ackDataIn[inCursor].write(!ackDataIn[inCursor].read()); // Toggle value

      inUse[outputToUse] = true;
      lastAccepted = inCursor;
      activeTransfers++;

      // Update the reservation if using wormhole routing.
      if(wormhole) {
        bool endOfPacket = dataIn[inCursor].read().endOfPacket();
        if(endOfPacket) {
          reservations[inCursor] = NO_RESERVATION;
          reserved[outputToUse] = false;
        }
        else {
          reservations[inCursor] = outputToUse;
          reserved[outputToUse] = true;
        }
      }
    }
  } // end for loop

  assert(activeTransfers >= 0);
  assert(activeTransfers <= numInputs);
  assert(activeTransfers <= numOutputs);
}

void ArbiterComponent::checkAcks() {
  // Pull down the valid signal for any outputs which have sent acknowledgements.
  for (int i = 0; i < numOutputs; i++) {
    if (ackDataOut[i].read()) {
      validDataOut[i].write(false);
      inUse[i] = false;
      activeTransfers--;
    }
  }

  assert(activeTransfers >= 0);
  assert(activeTransfers <= numInputs);
  assert(activeTransfers <= numOutputs);
}

void ArbiterComponent::newData() {
  newDataEvent.notify();
}

ArbiterComponent::ArbiterComponent(sc_module_name name, const ComponentID& ID, int inputs, int outputs, bool wormhole) :
    Component(name, ID)
{
	numInputs       = inputs;
	numOutputs      = outputs;
	this->wormhole  = wormhole;

	activeTransfers = 0;
	lastAccepted    = inputs - 1;

	dataIn          = new sc_in<AddressedWord>[inputs];
	validDataIn     = new sc_in<bool>[inputs];
	ackDataIn       = new sc_out<bool>[inputs];

	dataOut         = new sc_out<AddressedWord>[outputs];
	validDataOut    = new sc_out<bool>[outputs];
	ackDataOut      = new sc_in<bool>[outputs];

  inUse           = new bool[numOutputs];
  for(int i=0; i<numOutputs; i++) inUse[i] = false;

	if(wormhole) {
    reservations    = new int[numInputs];
    reserved        = new bool[numOutputs];

    for(int i=0; i<numInputs; i++)  reservations[i] = NO_RESERVATION;
    for(int i=0; i<numOutputs; i++) reserved[i] = false;
	}

	state = WAITING_FOR_DATA;

	SC_METHOD(arbiterLoop);
	sensitive << newDataEvent;
	dont_initialize();

	SC_METHOD(newData);
	for(int i=0; i<numInputs; i++) sensitive << validDataIn[i].pos();
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

  delete[] inUse;

	if(wormhole) {
	  delete[] reservations;
	  delete[] reserved;
	}
}
