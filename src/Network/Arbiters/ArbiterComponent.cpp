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

	ArbiterState state = currentState.read();

	if (state == STATE_INIT) {
		sc_core::sc_event_or_list& eventList = validDataIn[0].default_event() | validDataIn[0].default_event();

		for (int i = 1; i < numInputs; i++)
			eventList | validDataIn[i].default_event();

		currentState.write(STATE_STANDBY);
		next_trigger(eventList);
	} else if (state == STATE_STANDBY) {
		if (clock.negedge()) {
			arbitrate();
		} else {
			currentState.write(STATE_TRIGGERED);
			next_trigger(clock.negedge_event());
		}
	} else if (state == STATE_TRIGGERED) {
		arbitrate();
	} else if (state == STATE_TRANSFER) {
		int ackCount = 0;

		// Pull down the valid signal for any outputs which have sent acknowledgements.
		for (int i = 0; i < numOutputs; i++) {
			if (ackDataOut[i].read()) {
				validDataOut[i].write(false);
				ackCount++;
			}
		}

		if (activeTransfers.read() == ackCount) {
			activeTransfers.write(0);

			sc_core::sc_event_or_list& eventList = validDataIn[0].default_event() | validDataIn[0].default_event();

			for (int i = 1; i < numInputs; i++)
				eventList | validDataIn[i].default_event();

			currentState.write(STATE_STANDBY);
			next_trigger(eventList);
		} else {
			activeTransfers.write(activeTransfers.read() - ackCount);

			currentState.write(STATE_TRIGGERED);
			next_trigger(clock.negedge_event());
		}
	}
}

void ArbiterComponent::arbitrate() {
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

	if (outCursor == 0) {
		if (activeTransfers.read() == 0) {
			sc_core::sc_event_or_list& eventList = validDataIn[0].default_event() | validDataIn[0].default_event();

			for (int i = 1; i < numInputs; i++)
				eventList | validDataIn[i].default_event();

			currentState.write(STATE_STANDBY);
			next_trigger(eventList);
		} else {
			currentState.write(STATE_TRANSFER);
			next_trigger(clock.posedge_event());
		}
	} else {
		activeTransfers.write(activeTransfers.read() + outCursor);
		lastAccepted.write(lastAcc);

		currentState.write(STATE_TRANSFER);
		next_trigger(clock.posedge_event());
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

	currentState.write(STATE_INIT);
	activeTransfers.write(0);
	lastAccepted.write(numInputs - 1);

	SC_METHOD(arbitrateProcess);

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
