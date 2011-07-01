/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"
#include "../../TileComponent.h"
#include "../../../Datatype/AddressedWord.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Utility/Instrumentation/Stalls.h"

bool SendChannelEndTable::write(const DecodedInst& dec) {
	if (dec.operation() == InstructionMap::SETCHMAP) {
		updateMap(dec.immediate(), dec.result());
		return true;
	} else if (dec.operation() == InstructionMap::WOCHE) {
		waitUntilEmpty(dec.result());
		return true;
	} else if (dec.channelMapEntry() != Instruction::NO_CHANNEL && !channelMap[dec.channelMapEntry()].destination().isNullMapping()) {
		return executeMemoryOp(dec.channelMapEntry(), (MemoryRequest::MemoryOperation)dec.memoryOp(), dec.result());
	}

	return true;
}

bool SendChannelEndTable::write(const AddressedWord& data, MapIndex output) {
  unsigned int bufferToUse = (int)(channelMap[output].network());

  // We know it is safe to write to any buffer because we block the rest
  // of the pipeline if a buffer is full.
  assert(!buffers[bufferToUse].full());
  assert(output < CHANNEL_MAP_SIZE);

  buffers[bufferToUse].write(data);
  dataToSendEvent[bufferToUse].notify();
  bufferFillChanged.notify();
  mapEntries[bufferToUse].write(output);

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
	return channelMap[1].destination().getComponentID();
}

const sc_core::sc_event& SendChannelEndTable::stallChangedEvent() const {
  return bufferFillChanged;
}

void SendChannelEndTable::sendToCores()    {sendLoop(TO_CORES);}
void SendChannelEndTable::sendToMemories() {sendLoop(TO_MEMORIES);}
void SendChannelEndTable::sendOffTile()    {sendLoop(OFF_TILE);}

void SendChannelEndTable::sendLoop(unsigned int buffer) {
  if(ackOutput[buffer].read()) {
    // If we are receiving an acknowledgement, we have just sent some data.
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

    // Data can only be sent onto the network at the positive clock edge.
    if(!clock.posedge())
      next_trigger(clock.posedge_event());
    else
      send(buffer);
  }
}

void SendChannelEndTable::send(unsigned int buffer) {
  assert(!buffers[buffer].empty());
  assert(clock.posedge());

  MapIndex entry = mapEntries[buffer].peek();

  // If we don't have enough credits, abandon sending the data.
  if(!channelMap[entry].canSend()) {
    next_trigger(clock.posedge_event());
    return;
  }

  AddressedWord data = buffers[buffer].read();
  entry = mapEntries[buffer].read(); // Need to remove from the buffer after peeking earlier.

  if(DEBUG) cout << "Sending " << data.payload()
      << " from " << ChannelID(id, (int)entry) << " to "
      << data.channelID() << endl;

  output[buffer].write(data);
  validOutput[buffer].write(true);
  channelMap[entry].removeCredit();

  bufferFillChanged.notify();

  next_trigger(ackOutput[buffer].posedge_event());
}

/* Update an entry in the channel mapping table. */
void SendChannelEndTable::updateMap(MapIndex entry, int64_t newVal) {
	assert(entry < CHANNEL_MAP_SIZE);

	// All data sent to previous destinations must have been consumed before
	// we can change the destination. Stateless connections not using credits
	// are managed implicitely by the channel map table entry.

	if (!channelMap[entry].haveAllCredits())
		waitUntilEmpty(entry);

	// There are two separate scenarios to handle:
	// If the destination is a memory, the entry is referring to the first
	// bank in the memory group and carries the group size. If it is referring
	// to a core imitating a memory it needs to be set up like a regular
	// core to core connection.

	// Encoded entry format:
	// | Tile : 12 | Position : 8 | Channel : 4 | Memory group bits : 4 | Memory line bits : 4 |

	uint32_t encodedEntry = (uint32_t)newVal;
	bool multicast = (encodedEntry >> 28) & 0x1UL;
	uint newTile = (encodedEntry >> 20) & 0xFFUL;
	uint newPosition = (encodedEntry >> 12) & 0xFFUL;
	uint newChannel = (encodedEntry >> 8) & 0xFUL;
	uint newGroupBits = (encodedEntry >> 4) & 0xFUL;
	uint newLineBits = encodedEntry & 0xFUL;

	// Compute the global channel ID of the given output channel. Note that
	// since this is an output channel, the ID computation is different to
	// input channels.

	ChannelID sendChannel(newTile, newPosition, newChannel, multicast);
	ChannelID returnChannel(id, entry);

	if (sendChannel.isCore()) {
		// Destination is a core

		channelMap[entry].setCoreDestination(sendChannel);

		// Send port acquisition request
		// FetchLogic sends out the claims for entry 0

		if (entry != 0) {
			AddressedWord aw(Word(returnChannel.getData()), sendChannel);
			aw.setPortClaim(true, channelMap[entry].usesCredits());
			
			unsigned int bufferToUse = (int)(channelMap[entry].network());
			
			buffers[bufferToUse].write(aw);
			dataToSendEvent[bufferToUse].notify();
			bufferFillChanged.notify();
			mapEntries[bufferToUse].write(entry);
		}
	} else {
		// Destination is a memory

		channelMap[entry].setMemoryDestination(sendChannel, newGroupBits, newLineBits);

		/*
		// Send memory channel table setup request
		// FetchLogic sends out the claims for entry 0

		if (entry != 0) {
			MemoryRequest mr;
			mr.setHeaderSetTableEntry(returnChannel.getData());
			AddressedWord aw(mr, sendChannel);
			buffer.write(aw);
			mapEntries.write(entry);
		}
		*/
	}

	if (DEBUG)
		cout << this->name() << " updated map " << (int)entry << " to " << sendChannel << " [" << newGroupBits << "]" << endl;
}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty(MapIndex channel) {
  Instrumentation::stalled(id, true, Stalls::OUTPUT);
  waiting = true;
  bufferFillChanged.notify(); // No it didn't - use separate events?

  // Whilst the chosen channel does not have all of its credits, wait until a
  // new credit is received.
  while(!channelMap[channel].haveAllCredits()) {
    wait(creditsIn[0].default_event() | creditsIn[1].default_event() | creditsIn[2].default_event());
    // Wait a fraction longer to ensure the credit count is updated first.
    wait(sc_core::SC_ZERO_TIME);
  }

  Instrumentation::stalled(id, false);
  waiting = false;
  bufferFillChanged.notify(); // No it didn't - use separate events?
}

/* Execute a memory operation. */
bool SendChannelEndTable::executeMemoryOp(MapIndex entry, MemoryRequest::MemoryOperation memoryOp, int64_t data) {
	// Most messages we send will be one flit long, so will be the end of their
	// packets. However, packets for memory stores are two flits long (address
	// then data), so we need to mark this special case.

	ChannelID channel = channelMap[entry].destination();
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

	if (channelMap[entry].localMemory() && channelMap[entry].memoryGroupBits() > 0) {
		if (addressFlit) {
			increment = (uint32_t)data;
			increment &= (1UL << (channelMap[entry].memoryGroupBits() + channelMap[entry].memoryLineBits())) - 1UL;
			increment >>= channelMap[entry].memoryLineBits();
			channelMap[entry].setAddressIncrement(increment);
		} else {
			increment = channelMap[entry].getAddressIncrement();
		}
	}

	channel = channel.addPosition(increment);

	// Send request to memory

	AddressedWord aw(w, channel);
	aw.setEndOfPacket(endOfPacket);

	return write(aw, entry);
}

void SendChannelEndTable::receivedCredit(unsigned int buffer) {
	assert(validCredit[buffer].read());

  ChannelIndex targetCounter = creditsIn[buffer].read().channelID().getChannel();

  if(DEBUG) cout << "Received credit at " << ChannelID(id, targetCounter) << endl;

  channelMap[targetCounter].addCredit();
}

void SendChannelEndTable::creditFromCores()    {receivedCredit(TO_CORES);}
void SendChannelEndTable::creditFromMemories() {} // Memories don't send credits
void SendChannelEndTable::creditFromOffTile()  {receivedCredit(OFF_TILE);}

SendChannelEndTable::SendChannelEndTable(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    buffers(NUM_BUFFERS, OUT_CHANNEL_BUFFER_SIZE, string(name)),
    mapEntries(NUM_BUFFERS, OUT_CHANNEL_BUFFER_SIZE, string(name)),
    channelMap(CHANNEL_MAP_SIZE, ChannelMapEntry(ID)) {

  output      = new sc_out<AddressedWord>[NUM_BUFFERS];
  validOutput = new sc_out<bool>[NUM_BUFFERS];
  ackOutput   = new sc_in<bool>[NUM_BUFFERS];

  creditsIn   = new sc_in<AddressedWord>[NUM_BUFFERS];
  validCredit = new sc_in<bool>[NUM_BUFFERS];

  waiting = false;

  dataToSendEvent = new sc_core::sc_event[NUM_BUFFERS];

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
