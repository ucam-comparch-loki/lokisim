/*
 * FlowControlIn.cpp
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#include "FlowControlIn.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../TileComponents/TileComponent.h"

/* Put any new data into the buffer. Since we approved the request to send
 * data, it is known that there is enough room. */
void FlowControlIn::receivedData() {
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// This implementation is broken - but it is broken in a way the rest of the system depends on
	// Please do not change anything in the following lines without thorough testing of the on-chip network
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (execStateData != 2 && execCycleData != cycleCounter) {
		execCycleData = cycleCounter;
		execStateData = 0;
	}

	if (execStateData == 0 && clock.posedge()) {
		ackDataIn.write(false);

		// Allow time for the valid signal to be deasserted.
		triggerSignal.write(triggerSignal.read() + 1);
		execStateData = 1;
	} else if (execStateData == 1 && triggerSignal.event()) {
		if (validDataIn.read()) {
			if (dataIn.read().portClaim()) {
				// TODO: only accept the port claim when we have no credits left to send.

				// Set the return address so we can send flow control.
				returnAddress = dataIn.read().payload().toInt();
				useCredits = dataIn.read().useCredits();

				if (DEBUG)
					cout << "Channel " << channel << " was claimed by " << returnAddress << " [flow control " << (useCredits ? "enabled" : "disabled") << "]" << endl;

				// Allow core to wait until memory setup is complete if setting up a data connection

				if (!useCredits && dataIn.read().channelID().getChannel() >= 2) {
					// Wait until there is space in the buffer, if necessary
					if (bufferHasSpace.read()) {
						// Pass the value to the component.
						dataOut.write(dataIn.read().payload());

						execStateData = -1;
					} else {
						pendingData = dataIn.read().payload();

						execStateData = 2;
					}
				} else {
					execStateData = -1;
				}
			} else {
				// Wait until there is space in the buffer, if necessary
				if (bufferHasSpace.read()) {
					// Pass the value to the component.
					dataOut.write(dataIn.read().payload());

					execStateData = -1;
				} else {
					pendingData = dataIn.read().payload();

					execStateData = 2;
				}
			}

			if (useCredits) {
				numCredits++;
				assert(numCredits >= 0 && numCredits <= CHANNEL_END_BUFFER_SIZE);
			}

			// Acknowledge the data.
			ackDataIn.write(true);
		} else {
			execStateData = -1;
		}
	} else if (execStateData == 2 && bufferHasSpace.read()) {
		dataOut.write(pendingData);

		execStateData = -1;
	}
}

void FlowControlIn::updateReady() {
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// This implementation is broken - but it is broken in a way the rest of the system depends on
	// Please do not change anything in the following lines without thorough testing of the on-chip network
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (execStateCredits != 1 && execCycleCredits != cycleCounter) {
		execCycleCredits = cycleCounter;
		execStateCredits = 0;
	}

	if (execStateCredits == 0 && clock.posedge()) {
		if (numCredits > 0) {
			// This should not execute if credits are disabled
			assert(useCredits);

			// Send the new credit if someone is communicating with this port.
			if (!returnAddress.isNullMapping()) {
				AddressedWord aw(Word(1), returnAddress);
				creditsOut.write(aw);
				validCreditOut.write(true);

				numCredits--;
				assert(numCredits >= 0 && numCredits <= CHANNEL_END_BUFFER_SIZE);

				if(DEBUG) cout << "Sent credit from " << channel << " to " << returnAddress << endl;

				execStateCredits = 1;
			} else {
				execStateCredits = -1;
			}
		} else {
			execStateCredits = -1;
		}
	} else if (execStateCredits == 1 && ackCreditOut.read()) {
		// Deassert the valid signal when the acknowledgement arrives.
		validCreditOut.write(false);

		execStateCredits = -1;
	}
}

void FlowControlIn::clockProcess() {
	cycleCounter++;
}

FlowControlIn::FlowControlIn(sc_module_name name, const ComponentID& ID, const ChannelID& channelManaged) :
	Component(name, ID)
{
	channel = channelManaged;
	returnAddress = -1;
	useCredits = true;
	numCredits = 0;

	cycleCounter = 0;

	execCycleData = 0;
	execStateData = 0;
	execCycleCredits = 0;
	execStateCredits = 0;

	triggerSignal.write(0);

	SC_METHOD(receivedData);
	sensitive << clock.pos() << triggerSignal << bufferHasSpace;
	dont_initialize();

	SC_METHOD(updateReady);
	sensitive << clock.pos() << ackCreditOut;
	dont_initialize();

	SC_METHOD(clockProcess);
	sensitive << clock.pos();
	dont_initialize();
}
