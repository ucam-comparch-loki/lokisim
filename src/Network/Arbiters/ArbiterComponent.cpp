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
	while (true) {
		// Wait for something interesting to happen. We are sensitive to the
		// negative edge because data is put onto networks at the positive edge
		// and needs time to propagate through to these arbiters.

		if (inactiveCycles > 10) {
			assert(activeTransfers == 0);

			if (numInputs == 1) {
				wait(validDataIn[0].default_event());
			} else {
				sc_core::sc_event_or_list& eventList = validDataIn[0].default_event() | validDataIn[1].default_event();

				for (int i = 2; i < numInputs; i++)
					eventList | validDataIn[i].default_event();

				wait(eventList);
			}

			assert(sc_core::sc_time_stamp().value() % 1000 != 500);

			inactiveCycles = 0;
		}

		wait(clock.negedge_event());

		wait(sc_core::SC_ZERO_TIME);

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
			  // Find the next available output.
			  while((reserved[outCursor] || inUse[outCursor]) && outCursor < numOutputs)
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
		}

		assert(activeTransfers >= 0 && activeTransfers <= numInputs && activeTransfers <= numOutputs);

		if (activeTransfers > 0) {
			wait(clock.posedge_event());

			// Pull down the valid signal for any outputs which have sent acknowledgements.

			for (int i = 0; i < numOutputs; i++) {
				if (ackDataOut[i].read()) {
					validDataOut[i].write(false);
					inUse[i] = false;
					activeTransfers--;
				}
			}

			inactiveCycles = 0;
		} else {
			inactiveCycles++;
		}

		assert(activeTransfers >= 0 && activeTransfers <= numInputs && activeTransfers <= numOutputs);
	}
}

ArbiterComponent::ArbiterComponent(sc_module_name name, const ComponentID& ID, int inputs, int outputs, bool wormhole) :
    Component(name, ID)
{
	numInputs       = inputs;
	numOutputs      = outputs;
	this->wormhole  = wormhole;

	inactiveCycles  = 0x7FFF;
	activeTransfers = 0;
	lastAccepted    = inputs - 1;

	dataIn          = new sc_in<AddressedWord>[inputs];
	validDataIn     = new sc_in<bool>[inputs];
	ackDataIn       = new sc_out<bool>[inputs];

	dataOut         = new sc_out<AddressedWord>[outputs];
	validDataOut    = new sc_out<bool>[outputs];
	ackDataOut      = new sc_in<bool>[outputs];

	reservations    = new int[numInputs];
	reserved        = new bool[numOutputs];
	inUse           = new bool[numOutputs];

	for(int i=0; i<numInputs; i++) reservations[i] = NO_RESERVATION;
	for(int i=0; i<numOutputs; i++) {
	  reserved[i] = false;
	  inUse[i] = false;
	}

	SC_THREAD(mainLoop);

	end_module();
}

ArbiterComponent::~ArbiterComponent() {
	delete[] dataIn;
	delete[] validDataIn;
	delete[] ackDataIn;

	delete[] dataOut;
	delete[] validDataOut;
	delete[] ackDataOut;
}
