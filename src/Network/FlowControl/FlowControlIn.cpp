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

	if (execStateData == STATE_DATA_INIT) {
		// First call

		execStateData = STATE_DATA_STANDBY;
		next_trigger(validDataIn.default_event() & clock.posedge_event());
	} else if (execStateData == STATE_DATA_STANDBY) {
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

						execStateData = STATE_DATA_ACKNOWLEDGED;
						execTimeData = sc_core::sc_time_stamp().value();
						next_trigger(clock.posedge_event());
					} else {
						pendingData = dataIn.read().payload();

						execStateData = STATE_DATA_BUFFER_FULL;
						next_trigger(bufferHasSpace.posedge_event());
					}
				} else {
					execStateData = STATE_DATA_ACKNOWLEDGED;
					execTimeData = sc_core::sc_time_stamp().value();
					next_trigger(clock.posedge_event());
				}
			} else {
				// Wait until there is space in the buffer, if necessary
				if (bufferHasSpace.read()) {
					// Pass the value to the component.
					dataOut.write(dataIn.read().payload());

					execStateData = STATE_DATA_ACKNOWLEDGED;
					execTimeData = sc_core::sc_time_stamp().value();
					next_trigger(clock.posedge_event());
				} else {
					pendingData = dataIn.read().payload();

					execStateData = STATE_DATA_BUFFER_FULL;
					next_trigger(bufferHasSpace.posedge_event());
				}
			}

			if (useCredits) {
				numCredits++;
				assert(numCredits >= 0 && numCredits <= CHANNEL_END_BUFFER_SIZE);
			}

			// Acknowledge the data.
			ackDataIn.write(true);
		} else {
			execStateData = STATE_DATA_STANDBY;
			next_trigger(validDataIn.default_event() & clock.posedge_event());
		}
	} else if (execStateData == STATE_DATA_ACKNOWLEDGED) {
		if (execTimeData != sc_core::sc_time_stamp().value()) {
			ackDataIn.write(false);

			// Allow time for the valid signal to be deasserted.
			triggerSignal.write(triggerSignal.read() + 1);
			execStateData = STATE_DATA_STANDBY;
			next_trigger(triggerSignal.default_event());
		} else {
			execStateData = STATE_DATA_ACKNOWLEDGED;
			next_trigger(clock.posedge_event());
		}
	} else if (execStateData == STATE_DATA_BUFFER_FULL) {
		// Pass the value to the component.
		dataOut.write(pendingData);

		execStateData = STATE_DATA_ACKNOWLEDGED;
		execTimeData = sc_core::sc_time_stamp().value();
		next_trigger(clock.posedge_event());
	}

	// Not very clean but reduces the number of credit manager runs considerably

	creditsAvailable.write(numCredits > 0);
}

void FlowControlIn::updateReady() {
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// This implementation is broken - but it is broken in a way the rest of the system depends on
	// Please do not change anything in the following lines without thorough testing of the on-chip network
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (execStateCredits == STATE_CREDITS_INIT) {
		// First call

		execStateCredits = STATE_CREDITS_STANDBY;
		next_trigger(creditsAvailable.posedge_event() & clock.posedge_event());
	} else if (execStateCredits == STATE_CREDITS_STANDBY) {
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

				if (DEBUG)
					cout << "Sent credit from " << channel << " to " << returnAddress << endl;

				execStateCredits = STATE_CREDITS_SENT;
				next_trigger(ackCreditOut.posedge_event());
			} else {
				execStateCredits = STATE_CREDITS_STANDBY;
				next_trigger(creditsAvailable.posedge_event() & clock.posedge_event());
			}
		} else {
			execStateCredits = STATE_CREDITS_STANDBY;
			next_trigger(creditsAvailable.posedge_event() & clock.posedge_event());
		}
	} else if (execStateCredits == STATE_CREDITS_SENT) {
		// Deassert the valid signal when the acknowledgement arrives.
		validCreditOut.write(false);

		execStateCredits = STATE_CREDITS_STANDBY;
		next_trigger(creditsAvailable.posedge_event() & clock.posedge_event());
	}
}

FlowControlIn::FlowControlIn(sc_module_name name, const ComponentID& ID, const ChannelID& channelManaged) :
	Component(name, ID)
{
	channel = channelManaged;
	returnAddress = -1;
	useCredits = true;
	numCredits = 0;

	execStateData = STATE_DATA_INIT;
	execTimeData = 0;

	execStateCredits = STATE_CREDITS_INIT;

	ackDataIn.initialize(false);

	triggerSignal.write(0);
	creditsAvailable.write(false);

	SC_METHOD(receivedData);

	SC_METHOD(updateReady);
}
