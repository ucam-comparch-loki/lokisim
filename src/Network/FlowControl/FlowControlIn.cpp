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
	while (true) {
		wait(validDataIn.default_event());

		if (sc_core::sc_time_stamp().value() % 1000 != 0)
			wait(clock.posedge_event());

		wait(sc_core::SC_ZERO_TIME);  // Allow time for the valid signal to be deasserted.

		if (!validDataIn.read())
			continue;

		if (dataIn.read().portClaim()) {
			// TODO: only accept the port claim when we have no credits left to send.

			// Set the return address so we can send flow control.
			returnAddress = dataIn.read().payload().toInt();
			useCredits = dataIn.read().useCredits();

			if(DEBUG)
				cout << "Channel " << channel << " was claimed by " << returnAddress << " [flow control " << (useCredits ? "enabled" : "disabled") << "]" << endl;

			// Allow core to wait until memory setup is complete if setting up a data connection

			if (!useCredits && dataIn.read().channelID().getChannel() >= 2) {
				// Wait until there is space in the buffer, if necessary
				if (!bufferHasSpace.read())
					wait(bufferHasSpace.posedge_event());

				// Pass the value to the component.
				dataOut.write(dataIn.read().payload());
			}
		} else {
			// Wait until there is space in the buffer, if necessary
			if (!bufferHasSpace.read())
				wait(bufferHasSpace.posedge_event());

			// Pass the value to the component.
			dataOut.write(dataIn.read().payload());
		}

		if (useCredits) {
			numCredits++;
			assert(numCredits >= 0 && numCredits <= CHANNEL_END_BUFFER_SIZE);
		}

		// Acknowledge the data.
		ackDataIn.write(true);

		int inactiveCycles = 0;

		while (true) {
			// Wait for data to arrive.
			wait(clock.posedge_event());

			ackDataIn.write(false);

			wait(sc_core::SC_ZERO_TIME);  // Allow time for the valid signal to be deasserted.

			if (validDataIn.read()) {
				inactiveCycles = 0;
			} else {
				inactiveCycles++;
				if (inactiveCycles > 10)
					break;
				else
					continue;
			}

			if (dataIn.read().portClaim()) {
				// TODO: only accept the port claim when we have no credits left to send.

				// Set the return address so we can send flow control.
				returnAddress = dataIn.read().payload().toInt();
				useCredits = dataIn.read().useCredits();
				enableCreditUpdate.write(useCredits);

				if(DEBUG)
					cout << "Channel " << channel << " was claimed by " << returnAddress << " [flow control " << (useCredits ? "enabled" : "disabled") << "]" << endl;

				// Allow core to wait until memory setup is complete if setting up a data connection

				if (!useCredits && dataIn.read().channelID().getChannel() >= 2) {
					// Wait until there is space in the buffer, if necessary
					if (!bufferHasSpace.read())
						wait(bufferHasSpace.posedge_event());

					// Pass the value to the component.
					dataOut.write(dataIn.read().payload());
				}
			} else {
				// Wait until there is space in the buffer, if necessary
				if (!bufferHasSpace.read())
					wait(bufferHasSpace.posedge_event());

				// Pass the value to the component.
				dataOut.write(dataIn.read().payload());
			}

			if (useCredits) {
				numCredits++;
				assert(numCredits >= 0 && numCredits <= CHANNEL_END_BUFFER_SIZE);
			}

			// Acknowledge the data.
			ackDataIn.write(true);
		}
	}
}

void FlowControlIn::updateReady() {
	while (true) {
		//wait(enableCreditUpdate.posedge_event());

		while (true) {
			//if (!enableCreditUpdate.read())
			//	break;

			wait(clock.posedge_event());
			if (numCredits == 0)
				continue;

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

				// Deassert the valid signal when the acknowledgement arrives.
				wait(ackCreditOut.posedge_event());
				validCreditOut.write(false);
			}
		}
	}
}

FlowControlIn::FlowControlIn(sc_module_name name, const ComponentID& ID, const ChannelID& channelManaged) :
    Component(name, ID)
{
	channel = channelManaged;
	returnAddress = -1;
	useCredits = true;
	numCredits = 0;

	enableCreditUpdate.write(false);

	SC_THREAD(receivedData);
	SC_THREAD(updateReady);
}
