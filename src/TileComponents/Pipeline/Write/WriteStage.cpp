/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"
#include "../../Cluster.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Utility/InstructionMap.h"

ComponentID WriteStage::getSystemCallMemory() const {
	return scet.getSystemCallMemory();
}

void WriteStage::execute() {
	bool packetInProgress = false;
	bool fetchCommandPending = false;
	AddressedWord delayedFetchCommand;

	while(true) {
		// Wait for new data to arrive.

		wait(dataIn.default_event() | fromFetchLogic.default_event());

		// Whilst dealing with the new input, we are not idle.

		idle.write(false);

		// Enter a loop in case we receive data from both inputs at once.

		while (true) {
			if(!isStalled()) {
				if(dataIn.event()) {
					DecodedInst inst = dataIn.read();
					newInput(inst);
					packetInProgress = !endOfPacket;
				} else if(fromFetchLogic.event()) {
					if (packetInProgress) {
						assert(!fetchCommandPending);
						fetchCommandPending = true;
						delayedFetchCommand = fromFetchLogic.read();
					} else {
						scet.write(fromFetchLogic.read(), 0);
					}
				} else if (fetchCommandPending && !packetInProgress) {
					fetchCommandPending = false;
					scet.write(delayedFetchCommand, 0);
				} else {
					break;
				}
			}

			wait(clock.posedge_event());
		}

		idle.write(!fetchCommandPending && !packetInProgress);
	}
}

void WriteStage::newInput(DecodedInst& data) {
  if(DEBUG) cout << this->name() << " received Data: " << data.result()
                 << endl;

  // Put data into the send channel-end table.
  endOfPacket = scet.write(data);

  // Write to registers (they ignore the write if the index is invalid).
  if(InstructionMap::storesResult(data.operation())) {
    writeReg(data.destination(), data.result(),
             data.operation() == InstructionMap::IWTR);
  }

  // Do we need to say we are stalling because of output if the SCET is full?

}

void WriteStage::updateReady() {
  // Write our current stall status.
  readyOut.write(!isStalled());

  if(DEBUG && isStalled() && readyOut.read()) {
    cout << this->name() << " stalled." << endl;
  }

  // Wait until some point late in the cycle, so we know that any operations
  // will have completed.
  next_trigger(scet.stallChangedEvent());
}

bool WriteStage::isStalled() const {
  return scet.full();
}

void WriteStage::writeReg(RegisterIndex reg, int32_t value, bool indirect) const {
  parent()->writeReg(reg, value, indirect);
}

WriteStage::WriteStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    scet("scet", ID) {

	//TODO: Replace this hack with something more sensible
	endOfPacket = false;

  // Connect the SCET to the network.
  scet.clock(clock);
  scet.output(output);
  scet.validOutput(validOutput);
  scet.ackOutput(ackOutput);
  scet.creditsIn(creditsIn);
  scet.validCredit(validCredit);

  SC_THREAD(execute);
}
