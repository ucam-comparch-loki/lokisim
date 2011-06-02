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

			// FIXME: a request may be granted, but then blocked by flow control.
			// Another, later request may still be allowed to send. Seems unfair.

			if (!validDataOut[outCursor].read()) {
				dataOut[outCursor].write(dataIn[inCursor].read());
				validDataOut[outCursor].write(true);
				ackDataIn[inCursor].write(!ackDataIn[inCursor].read()); // Toggle value
				lastAccepted = inCursor;
				activeTransfers++;
			}

			outCursor++;
			if (outCursor == numOutputs)
				break;
		}

		assert(activeTransfers >= 0 && activeTransfers <= numInputs && activeTransfers <= numOutputs);

		if (activeTransfers > 0) {
			wait(clock.posedge_event());

			// Pull down the valid signal for any outputs which have sent acknowledgements.

			for (int i = 0; i < numOutputs; i++) {
				if (ackDataOut[i].read()) {
					validDataOut[i].write(false);
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

ArbiterComponent::ArbiterComponent(sc_module_name name, const ComponentID& ID, int inputs, int outputs) :
    Component(name, ID)
{
	numInputs       = inputs;
	numOutputs      = outputs;

	inactiveCycles  = 0x7FFF;
	activeTransfers = 0;
	lastAccepted    = inputs - 1;

	dataIn          = new sc_in<AddressedWord>[inputs];
	validDataIn     = new sc_in<bool>[inputs];
	ackDataIn       = new sc_out<bool>[inputs];

	dataOut         = new sc_out<AddressedWord>[outputs];
	validDataOut    = new sc_out<bool>[outputs];
	ackDataOut      = new sc_in<bool>[outputs];

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
