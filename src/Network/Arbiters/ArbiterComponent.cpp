/*
 * ArbiterComponent.cpp
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#include "ArbiterComponent.h"
#include "../../Datatype/AddressedWord.h"

void ArbiterComponent::arbitrateProcess() {
    // Wait for something interesting to happen. We are sensitive to the
    // negative edge because data is put onto networks at the positive edge
    // and needs time to propagate through to these arbiters.

	if (clock.negedge()) {
		int inCursor = lastAccepted.read();
		int outCursor = 0;
		int lastAcc = 0;

		for (int i = 0; i < numInputs; i++) {
			// Request: numinput -> endofpacket

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
				lastAcc = inCursor;
			}

			outCursor++;
			if (outCursor == numOutputs)
				break;
		}

		if (outCursor != 0)
			lastAccepted.write(lastAcc);
	} else {
		// Pull down the valid signal for any outputs which have sent acknowledgements.
		for (int i = 0; i < numOutputs; i++)
			if (ackDataOut[i].read())
				validDataOut[i].write(false);
	}
}

ArbiterComponent::ArbiterComponent(sc_module_name name, const ComponentID& ID, int inputs, int outputs) :
    Component(name, ID)
{
	numInputs    = inputs;
	numOutputs   = outputs;

	dataIn       = new sc_in<AddressedWord>[inputs];
	validDataIn  = new sc_in<bool>[inputs];
	ackDataIn    = new sc_out<bool>[inputs];

	dataOut      = new sc_out<AddressedWord>[outputs];
	validDataOut = new sc_out<bool>[outputs];
	ackDataOut   = new sc_in<bool>[outputs];

	inputs = numInputs;
	outputs = numOutputs;

	lastAccepted.write(numInputs - 1);

	SC_METHOD(arbitrateProcess);
	sensitive << clock.neg() << clock.pos();
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
}
