/*
 * WriteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "WriteStage.h"
#include "../../Cluster.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Utility/Instrumentation/Stalls.h"

const DecodedInst& WriteStage::currentInstruction() const {
  return currentInst;
}

void WriteStage::execute() {
	bool packetInProgress = false;

	// TODO: turn this into an SC_METHOD once the output buffers are gone.
	// Need to handle WOCHE (or would this happen in ExecuteStage?).
	while(true) {
		// Wait for new data to arrive.
		wait(dataIn.default_event());

    if(!isStalled()) {
      DecodedInst instruction = dataIn.read();
      newInput(instruction);
      packetInProgress = !endOfPacket;
    }

		idle.write(!packetInProgress);

    // Invalidate the current instruction at the start of the next cycle so
    // we don't forward old data.
    wait(clock.posedge_event());
    currentInst.destination(0);
	}
}

void WriteStage::newInput(DecodedInst& data) {
  if(DEBUG) cout << this->name() << " received Data: " << data.result()
                 << endl;

  currentInst = data;

  // We can't forward data if this is an indirect write because we don't
  // yet know where the data will be written.
  bool indirect = (data.operation() == InstructionMap::IWTR);
  if(indirect) currentInst.destination(0);

  // Put data into the send channel-end table.
  endOfPacket = scet.write(data);

  // Write to registers (they ignore the write if the index is invalid).
  if(InstructionMap::storesResult(data.operation()))
    writeReg(data.destination(), data.result(), indirect);
}

void WriteStage::updateReady() {
  bool ready = !isStalled();

  if(DEBUG && !ready && readyOut.read()) {
    cout << this->name() << " stalled." << endl;
  }

  // Write our current stall status.
  if(ready != readyOut.read()) {
    readyOut.write(ready);
    Instrumentation::stalled(id, !ready, Stalls::OUTPUT);
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
    scet("scet", ID, &(parent()->channelMapTable)) {

  static const unsigned int NUM_BUFFERS = CORE_OUTPUT_PORTS;

  output      = new sc_out<AddressedWord>[NUM_BUFFERS];
  validOutput = new sc_out<bool>[NUM_BUFFERS];

  creditsIn   = new sc_in<AddressedWord>[NUM_BUFFERS];
  validCredit = new sc_in<bool>[NUM_BUFFERS];

	//TODO: Replace this hack with something more sensible
	endOfPacket = false;

  // Connect the SCET to the network.
  scet.clock(clock);

  for(unsigned int i=0; i<NUM_BUFFERS; i++) {
    scet.output[i](output[i]);
    scet.validOutput[i](validOutput[i]);
    scet.creditsIn[i](creditsIn[i]);
    scet.validCredit[i](validCredit[i]);
  }

  SC_THREAD(execute);
}

WriteStage::~WriteStage() {
  delete[] output;
  delete[] validOutput;

  delete[] creditsIn;
  delete[] validCredit;
}
