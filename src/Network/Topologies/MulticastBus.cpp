/*
 * MulticastBus.cpp
 *
 *  Created on: 8 Mar 2011
 *      Author: db434
 */

#include "MulticastBus.h"
#include "../../Datatype/AddressedWord.h"

void MulticastBus::busLoop() {
  switch(state) {
    case WAITING_FOR_DATA : {
      if(clock.posedge()) {
        // Wait for a delta cycle, because the valid signal is deasserted on
        // the clock edge.
        next_trigger(sc_core::SC_ZERO_TIME);
      }
      else if(!validDataIn[0].read()) {
        // It turns out that there wasn't actually more data: the valid signal
        // was deasserted on the clock edge.
        next_trigger(validDataIn[0].posedge_event());
      }
      else {
        // There definitely is data: send it.
        DataType data = dataIn[0].read();

        outputUsed = getDestinations(data.channelID());
        assert(outputUsed != 0);

        for(int i=0; i<8; i++) {  // 8 = number of bits in PortIndex... increase this?
          if((outputUsed >> i) & 1) {
            dataOut[i].write(data);
            validDataOut[i].write(true);
          }
        }

        next_trigger(receivedAck);
        state = WAITING_FOR_ACK;
      }

      break;
    }

    case WAITING_FOR_ACK : {
      // The data has been consumed, so is no longer valid.
      for(int i=0; i<8; i++) {  // 8 = number of bits in PortIndex... increase this?
        if(ackDataOut[i].event()) {
          // If we're receiving an ack, we should have sent data on that port.
          assert((outputUsed >> i) & 1);
          outputUsed &= ~(1 << i);  // Clear the bit

          validDataOut[i].write(false);
        }
      }

      if(outputUsed == 0) {
        // All acknowledgements have been received, so now it is time to
        // acknowledge the input data.
        ackDataIn[0].write(true);

        next_trigger(clock.posedge_event());
        state = SENT_ACK;
      }
      else {
        // Still more acknowledgements to wait for.
        next_trigger(receivedAck);
      }

      break;
    }

    case SENT_ACK : {
      assert(ackDataIn[0].read());

      ackDataIn[0].write(false);

      if(!validDataIn[0].read()) {
        next_trigger(validDataIn[0].posedge_event());
        state = WAITING_FOR_DATA;
      }
      else {
        // The valid signal is still high at the start of the next clock cycle.
        // This means there may be more data to consume.
        next_trigger(sc_core::SC_ZERO_TIME);
        state = WAITING_FOR_DATA;
      }

      break;
    }
  } // end switch
}

void MulticastBus::ackArrived() {
  receivedAck.notify();
}

PortIndex MulticastBus::getDestinations(const ChannelID& address) const {
  // In practice, we would probably only allow multicast addresses, but
  // allowing both options for the moment makes testing easier.

  if(level == COMPONENT && address.isMulticast()) {
    // The address is already correctly encoded.
    return getDestination(address);
  }
  else {
    // If it is not a multicast address, there is only one destination.
    // Shift a single bit to the correct position.
    return 1 << getDestination(address);
  }
}

MulticastBus::MulticastBus(const sc_module_name& name, const ComponentID& ID, int numOutputs,
                           HierarchyLevel level, Dimension size, int firstOutput) :
    Bus(name, ID, numOutputs, level, size, firstOutput) {

//  creditsIn      = new CreditInput[numOutputs];
//  validCreditIn  = new ReadyInput[numOutputs];
//  ackCreditIn    = new ReadyOutput[numOutputs];
//
//  creditsOut     = new CreditOutput[1];
//  validCreditOut = new ReadyOutput[1];
//  ackCreditOut   = new ReadyInput[1];

  SC_METHOD(ackArrived);
  for(int i=0; i<numOutputs; i++) sensitive << ackDataOut[i];
  dont_initialize();
}

MulticastBus::~MulticastBus() {
//  delete[] creditsIn;
//  delete[] validCreditIn;
//  delete[] ackCreditIn;
//
//  delete[] creditsOut;
//  delete[] validCreditOut;
//  delete[] ackCreditOut;
}
