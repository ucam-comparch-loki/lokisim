/*
 * ArbiterComponent.cpp
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

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

      // Try to accept more data if all outputs are not already busy.
      if(activeTransfers < outputs) arbitrate();

      if(activeTransfers > 0) {
        state = WAITING_FOR_ACKS;
        next_trigger(clock.posedge_event());
      }
      else {
        // There are no acknowledgements to wait for, so wait until data arrives.
        next_trigger(newDataEvent);
      }

      break;
    }

    case WAITING_FOR_ACKS : {

      // Now that the clock edge has arrived, pull down any valid data signals.
      for(unsigned int i=0; i<outputs; i++) {
        if(!inUse[i] && validDataOut[i].read()) {
          validDataOut[i].write(false);

          int dataSource = ackDestinations[i];
          alreadySeen[dataSource] = false;

          activeTransfers--;

          assert(activeTransfers >= 0);
          assert(activeTransfers <= inputs);
          assert(activeTransfers <= outputs);
        }
      }

      state = WAITING_FOR_DATA;

      // Wait until data arrives, or until the next clock edge if it is already
      // here.
      if(activeTransfers > 0 || haveData()) next_trigger(clock.negedge_event());
      else                                  next_trigger(newDataEvent);

      break;
    }
  } // end switch
}

void ArbiterComponent::arbitrate() {
  // Currently implements a round-robin arbitration scheme.
  // TODO: make use of the classes in Arbitration so the behaviour can easily
  // be swapped in and out.

  unsigned int inCursor = lastAccepted;
  unsigned int outCursor = 0;

  for (unsigned int i = 0; i < inputs; i++) {
    // Request: numInput -> endOfPacket

    inCursor++;
    if (inCursor == inputs)
      inCursor = 0;

    if (!validDataIn[inCursor].read() || alreadySeen[inCursor])
      continue;

    // Determine which output to send the data to. If this arbiter makes use
    // of wormhole routing, some outputs may be reserved.
    unsigned int outputToUse;
    if(wormhole && (reservations[inCursor] != NO_RESERVATION)) {
      outputToUse = reservations[inCursor];
    }
    else {
      // Find the next available output (skip over any which are in use
      // or reserved by wormhole-routed packets).
      while((inUse[outCursor] || (wormhole && reserved[outCursor]))
         && (outCursor < outputs))
        outCursor++;

      outputToUse = outCursor;
    }

    // Don't exit the loop if we have now been through all outputs. There may
    // still be reservations which can be filled.
    if(outputToUse >= outputs) continue;

    // Send the data, if appropriate.
    if (!inUse[outputToUse]) {
      dataOut[outputToUse].write(dataIn[inCursor].read());
      validDataOut[outputToUse].write(true);

      ackDestinations[outputToUse] = inCursor;
      alreadySeen[inCursor] = true;

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
  assert(activeTransfers <= inputs);
  assert(activeTransfers <= outputs);
}

bool ArbiterComponent::haveData() {
  return numValidInputs > 0;
}

void ArbiterComponent::receivedAck() {
  for (unsigned int i = 0; i < outputs; i++) {
    if (inUse[i] && ackDataOut[i].read()) {
      // Recall which input the data came from so we know where to send the
      // acknowledgement.
      int dataSource = ackDestinations[i];

      assert(validDataIn[dataSource].read());

      // Acknowledge the input data now it has been consumed, but don't
      // deassert the validData signal until the positive clock edge.
      sendAckEvent[dataSource].notify();

      // Signals that this validData signal should be pulled down later.
      inUse[i] = false;
    }
  }
}

// Set the acknowledgement signal high when sendAckEvent is triggered, and
// low at the next clock edge.
void ArbiterComponent::sendAck(PortIndex input) {
  if(clock.posedge()) {
    ackDataIn[input].write(false);
    next_trigger(sendAckEvent[input]);
  }
  else {
    ackDataIn[input].write(true);
    next_trigger(clock.posedge_event());
  }
}

// Keep track of the number of input ports which currently have valid data,
// so we can quickly check whether any of them have data without looping
// through all of them.
void ArbiterComponent::newData(PortIndex input) {
  if(validDataIn[input].read()) {
    newDataEvent.notify();
    numValidInputs++;
    next_trigger(validDataIn[input].negedge_event());
  }
  else {
    numValidInputs--;
    next_trigger(validDataIn[input].posedge_event());
  }
}

unsigned int ArbiterComponent::numInputs()  const {return inputs;}
unsigned int ArbiterComponent::numOutputs() const {return outputs;}

ArbiterComponent::ArbiterComponent(const sc_module_name& name, const ComponentID& ID, int inputs, int outputs, bool wormhole) :
    Component(name, ID),
    inputs(inputs),
    outputs(outputs),
    wormhole(wormhole)
{

  activeTransfers = 0;
  numValidInputs  = inputs;  // Will be taken down to 0 immediately
  lastAccepted    = inputs - 1;

  dataIn          = new sc_in<AddressedWord>[inputs];
  validDataIn     = new sc_in<bool>[inputs];
  ackDataIn       = new sc_out<bool>[inputs];

  dataOut         = new sc_out<AddressedWord>[outputs];
  validDataOut    = new sc_out<bool>[outputs];
  ackDataOut      = new sc_in<bool>[outputs];

  sendAckEvent    = new sc_core::sc_event[inputs];

  inUse           = new bool[outputs];
  for(int i=0; i<outputs; i++) inUse[i] = false;

  ackDestinations = new int[outputs];

  alreadySeen     = new bool[inputs];
  for(int i=0; i<inputs; i++) alreadySeen[i] = false;

  if(wormhole) {
    reservations    = new unsigned int[inputs];
    reserved        = new bool[outputs];

    for(int i=0; i<inputs; i++)  reservations[i] = NO_RESERVATION;
    for(int i=0; i<outputs; i++) reserved[i] = false;
  }

  state = WAITING_FOR_DATA;

  // Start arbitrating when data arrives.
  SC_METHOD(arbiterLoop);
  sensitive << newDataEvent;
  dont_initialize();

  SC_METHOD(receivedAck);
  for(int i=0; i<outputs; i++) sensitive << ackDataOut[i].pos();
  dont_initialize();

  // Generate a method to watch each input port, keeping track of when there is
  // at least one port with valid data.
  for(int i=0; i<inputs; i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread

    // Create the method.
    sc_spawn(sc_bind(&ArbiterComponent::newData, this, i), 0, &options);
  }

  // Generate a method to send acknowledgements on each input port.
  for(int i=0; i<inputs; i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread
    options.dont_initialize();  // Only execute when triggered
    options.set_sensitivity(&(sendAckEvent[i])); // Sensitive to this event

    // Create the method.
    sc_spawn(sc_bind(&ArbiterComponent::sendAck, this, i), 0, &options);
  }

  end_module();
}

ArbiterComponent::~ArbiterComponent() {
  delete[] dataIn;  delete[] validDataIn;   delete[] ackDataIn;
  delete[] dataOut; delete[] validDataOut;  delete[] ackDataOut;

  delete[] sendAckEvent;

  delete[] inUse;
  delete[] ackDestinations;
  delete[] alreadySeen;

  if(wormhole) {
    delete[] reservations;
    delete[] reserved;
  }
}
