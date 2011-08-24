/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"
#include "WriteStage.h"
#include "../ChannelMapTable.h"
#include "../../TileComponent.h"
#include "../../../Datatype/AddressedWord.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/Instrumentation/Stalls.h"

bool SendChannelEndTable::write(const DecodedInst& dec) {
	if(dec.operation() == InstructionMap::SETCHMAP) {
	  // Send out a port claim message if we have just set up a new connection.
	  createPortClaim(dec.immediate());
	}
  else if (dec.operation() == InstructionMap::WOCHE) {
		waitUntilEmpty(dec.result());
	} else if (dec.channelMapEntry() != Instruction::NO_CHANNEL && !dec.networkDestination().isNullMapping()) {
		return executeMemoryOp(dec.channelMapEntry(), (MemoryRequest::MemoryOperation)dec.memoryOp(), dec.result(), dec.networkDestination());
	}
	
	return true;
}

bool SendChannelEndTable::write(const AddressedWord& data, MapIndex output) {
  unsigned int bufferToUse = (int)channelMapTable->getNetwork(output);

  // We know it is safe to write to any buffer because we block the rest
  // of the pipeline if a buffer is full.
  assert(!buffers[bufferToUse].full());
  assert(output < CHANNEL_MAP_SIZE);

  buffers[bufferToUse].write(BufferedInfo(data, output));
  dataToSendEvent[bufferToUse].notify();
  bufferFillChanged.notify();

  if(DEBUG) cout << this->name() << " wrote " << data
                 << " to output buffer " << bufferToUse << endl;

  return data.endOfPacket();
}

bool SendChannelEndTable::full() const {
  bool full = false;
  for(unsigned int i=0; i<buffers.size(); i++) {
    if(buffers[i].full()) {
      full = true;
      break;
    }
  }

  return full || waiting;
}

ComponentID SendChannelEndTable::getSystemCallMemory() const {
	//TODO: Implement this in a tidy way
	return channelMapTable->read(1).getComponentID();
}

const sc_event& SendChannelEndTable::stallChangedEvent() const {
  return bufferFillChanged;
}

void SendChannelEndTable::sendToCores()    {sendLoop(TO_CORES);}
void SendChannelEndTable::sendToMemories() {sendLoop(TO_MEMORIES);}
void SendChannelEndTable::sendOffTile()    {sendLoop(OFF_TILE);}

void SendChannelEndTable::sendLoop(unsigned int buffer) {
  if(ackOutput[buffer].posedge()) {
    // If we are receiving an acknowledgement, we have just sent some data.
    assert(validOutput[buffer].read());
    validOutput[buffer].write(false);

    if(buffers[buffer].empty())        // No data to send.
      next_trigger(dataToSendEvent[buffer]);
    else if(clock.posedge())  // Have data and it's time to send it.
      send(buffer);
    else                      // Have data but it's not time to send it.
      next_trigger(clock.posedge_event());
  }
  else {
    // If we are not receiving an acknowledgement, we must have data to send.
    assert(!buffers[buffer].empty());

//    // Data can only be sent onto the network at the positive clock edge.
//    if(!clock.posedge())
//      next_trigger(clock.posedge_event());
//    else
      send(buffer);
  }
}

void SendChannelEndTable::send(unsigned int buffer) {
  assert(!buffers[buffer].empty());

  MapIndex entry = buffers[buffer].peek().second;

  // If we don't have enough credits, abandon sending the data.
  if(!channelMapTable->canSend(entry)) {
    next_trigger(clock.posedge_event());
    return;
  }

  AddressedWord data = buffers[buffer].read().first;

  if(DEBUG) cout << "Sending " << data.payload()
      << " from " << ChannelID(id, (int)entry) << " to "
      << data.channelID() << endl;

  output[buffer].write(data);
  validOutput[buffer].write(true);
  channelMapTable->removeCredit(entry);

  bufferFillChanged.notify();

  next_trigger(ackOutput[buffer].posedge_event());
}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty(MapIndex channel) {
  Instrumentation::stalled(id, true, Stalls::OUTPUT);
  waiting = true;
  bufferFillChanged.notify(); // No it didn't - use separate events?

  // Wait until the channel's credit counter reaches its maximum value, if it
  // is using credits.
  channelMapTable->waitForAllCredits(channel);

  Instrumentation::stalled(id, false);
  waiting = false;
  bufferFillChanged.notify(); // No it didn't - use separate events?
}

/* Execute a memory operation. */
bool SendChannelEndTable::executeMemoryOp(MapIndex entry,
                                          MemoryRequest::MemoryOperation memoryOp,
                                          int64_t data,
                                          ChannelID destination) {
	// Most messages we send will be one flit long, so will be the end of their
	// packets. However, packets for memory stores are two flits long (address
	// then data), so we need to mark this special case.

	ChannelID channel = destination;
	bool addressFlit = false;
	bool endOfPacket = true;
	Word w;

	if (memoryOp != MemoryRequest::NONE) {
		// Generate a special memory request if we are doing a load/store/etc.

		// Generate a memory request using the address from the ALU and the operation
		// supplied by the decoder. The memory request will be sent to a memory and
		// will result in an operation being carried out there.

		w = MemoryRequest(memoryOp, (uint32_t)data);

		if (memoryOp == MemoryRequest::LOAD_B || memoryOp == MemoryRequest::LOAD_HW || memoryOp == MemoryRequest::LOAD_W || memoryOp == MemoryRequest::STORE_W || memoryOp == MemoryRequest::STORE_HW || memoryOp == MemoryRequest::STORE_B)
			addressFlit = true;

		if (memoryOp == MemoryRequest::STORE_W || memoryOp == MemoryRequest::STORE_HW || memoryOp == MemoryRequest::STORE_B)
			endOfPacket = false;
	} else {
		w = Word(data);
	}

	// Adjust destination channel based on memory configuration if necessary

	uint32_t increment = 0;

	// We want to access lots of information from the channel map table, so get
	// the entire entry.
	ChannelMapEntry& channelMapEntry = channelMapTable->getEntry(entry);

	if (channelMapEntry.localMemory() && channelMapEntry.memoryGroupBits() > 0) {
    if (addressFlit) {
      increment = (uint32_t)data;
      increment &= (1UL << (channelMapEntry.memoryGroupBits() + channelMapEntry.memoryLineBits())) - 1UL;
      increment >>= channelMapEntry.memoryLineBits();
      channelMapEntry.setAddressIncrement(increment);
    } else {
      increment = channelMapEntry.getAddressIncrement();
    }
  }

	channel = channel.addPosition(increment);

	// Send request to memory

	AddressedWord aw(w, channel);
	aw.setEndOfPacket(endOfPacket);

	return write(aw, entry);
}

void SendChannelEndTable::createPortClaim(MapIndex channel) {
  // The destination channel end to claim (a table read isn't necessary here -
  // the information could be extracted from the instruction).
  ChannelID destination = channelMapTable->read(channel);

  // The address to send credits back to (if applicable).
  ChannelID returnChannel(id, channel);

  if (destination.isCore()) {
    // Send port acquisition request
    // FetchLogic sends out the claims for entry 0
    if (channel != 0) {
      AddressedWord aw(returnChannel, destination);
      aw.setPortClaim(true, channelMapTable->getEntry(channel).usesCredits());
      unsigned int bufferToUse = (int)(channelMapTable->getNetwork(channel));
      buffers[bufferToUse].write(BufferedInfo(aw, channel));
      dataToSendEvent[bufferToUse].notify();
      bufferFillChanged.notify();
    }
  }
}

void SendChannelEndTable::receivedCredit(unsigned int buffer) {
  assert(validCredit[buffer].read());

  ChannelIndex targetCounter = creditsIn[buffer].read().channelID().getChannel();

  if(DEBUG) cout << "Received credit at " << ChannelID(id, targetCounter) << endl;

  channelMapTable->addCredit(buffer);
}

void SendChannelEndTable::creditFromCores()    {receivedCredit(TO_CORES);}
void SendChannelEndTable::creditFromMemories() {} // Memories don't send credits
void SendChannelEndTable::creditFromOffTile()  {receivedCredit(OFF_TILE);}

WriteStage* SendChannelEndTable::parent() const {
  return static_cast<WriteStage*>(this->get_parent());
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name, const ComponentID& ID, ChannelMapTable* cmt) :
    Component(name, ID),
    buffers(NUM_BUFFERS, OUT_CHANNEL_BUFFER_SIZE, string(name)) {

  channelMapTable = cmt;

  output      = new sc_out<AddressedWord>[NUM_BUFFERS];
  validOutput = new sc_out<bool>[NUM_BUFFERS];
  ackOutput   = new sc_in<bool>[NUM_BUFFERS];

  creditsIn   = new sc_in<AddressedWord>[NUM_BUFFERS];
  validCredit = new sc_in<bool>[NUM_BUFFERS];

  waiting = false;

  dataToSendEvent = new sc_event[NUM_BUFFERS];

  SC_METHOD(sendToCores);        sensitive << dataToSendEvent[0];   dont_initialize();
  SC_METHOD(sendToMemories);     sensitive << dataToSendEvent[1];   dont_initialize();
  SC_METHOD(sendOffTile);        sensitive << dataToSendEvent[2];   dont_initialize();

  SC_METHOD(creditFromCores);    sensitive << validCredit[0].pos(); dont_initialize();
  SC_METHOD(creditFromMemories); sensitive << validCredit[1].pos(); dont_initialize();
  SC_METHOD(creditFromOffTile);  sensitive << validCredit[2].pos(); dont_initialize();

}

SendChannelEndTable::~SendChannelEndTable() {
  delete[] output;
  delete[] validOutput;
  delete[] ackOutput;

  delete[] creditsIn;
  delete[] validCredit;

  delete[] dataToSendEvent;
}
