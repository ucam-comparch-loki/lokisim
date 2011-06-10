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
  // We know it is safe to write to any buffer because we block the rest
  // of the pipeline if a buffer is full.
  assert(!buffer.full());
  assert(output < CHANNEL_MAP_SIZE);

  buffer.write(data);
  dataToSendEvent.notify();
  mapEntries.write(output);

  if(DEBUG) cout << this->name() << " wrote " << data
                 << " to output buffer" << endl;

  return data.endOfPacket();
}

bool SendChannelEndTable::full() const {
  return buffer.full() || waiting;
}

ComponentID SendChannelEndTable::getSystemCallMemory() const {
	//TODO: Implement this in a tidy way
	return channelMap[1].destination().getComponentID();
}

void SendChannelEndTable::sendLoop() {
  if(ackOutput.read()) {
    // If we are receiving an acknowledgement, we have just sent some data.
    validOutput.write(false);

    if(buffer.empty())        // No data to send.
      next_trigger(dataToSendEvent);
    else if(clock.posedge())  // Have data and it's time to send it.
      send();
    else                      // Have data but it's not time to send it.
      next_trigger(clock.posedge_event());
  }
  else {
    // If we are not receiving an acknowledgement, we must have data to send.
    assert(!buffer.empty());

    // Data can only be sent onto the network at the positive clock edge.
    if(!clock.posedge())
      next_trigger(clock.posedge_event());
    else
      send();
  }
}

void SendChannelEndTable::send() {
  assert(!buffer.empty());
  assert(clock.posedge());

  MapIndex entry = mapEntries.peek();

  // If we don't have enough credits, abandon sending the data.
  if(!channelMap[entry].canSend()) {
    next_trigger(clock.posedge_event());
    return;
  }

  AddressedWord data = buffer.read();
  entry = mapEntries.read(); // Need to remove from the buffer after peeking earlier.

  if(DEBUG) cout << "Sending " << data.payload()
      << " from " << ChannelID(id, (int)entry) << " to "
      << data.channelID() << endl;

  output.write(data);
  validOutput.write(true);
  channelMap[entry].removeCredit();

  next_trigger(ackOutput.posedge_event());
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
	uint newTile = encodedEntry >> 20;
	uint newPosition = (encodedEntry >> 12) & 0xFFUL;
	uint newChannel = (encodedEntry >> 8) & 0xFUL;
	uint newGroupBits = (encodedEntry >> 4) & 0xFUL;
	uint newLineBits = encodedEntry & 0xFUL;

	// Compute the global channel ID of the given output channel. Note that
	// since this is an output channel, the ID computation is different to
	// input channels.

	ChannelID sendChannel(newTile, newPosition, newChannel);
	ChannelID returnChannel(id, entry);

	if (sendChannel.isCore()) {
		// Destination is a core

		channelMap[entry].setCoreDestination(sendChannel);

		// Send port acquisition request
		// FetchLogic sends out the claims for entry 0

		if (entry != 0) {
			AddressedWord aw(Word(returnChannel.getData()), sendChannel);
			aw.setPortClaim(true, true);
			buffer.write(aw);
			dataToSendEvent.notify();
			mapEntries.write(entry);
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

  // Whilst the chosen channel does not have all of its credits, wait until a
  // new credit is received.
  while(!channelMap[channel].haveAllCredits()) {
    wait(creditsIn.default_event());
    // Wait a fraction longer to ensure the credit count is updated first.
    wait(sc_core::SC_ZERO_TIME);
  }

  Instrumentation::stalled(id, false);
  waiting = false;
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

void SendChannelEndTable::receivedCredit() {
    // Note: this may not work if we have multiple tiles: it relies on all of the
    // addresses being neatly aligned.
    ChannelIndex targetCounter = creditsIn.read().channelID().getChannel();

    if(DEBUG) cout << "Received credit at " << ChannelID(id, targetCounter) << endl;

    channelMap[targetCounter].addCredit();
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    buffer(CHANNEL_END_BUFFER_SIZE, string(name)),
    mapEntries(CHANNEL_END_BUFFER_SIZE, string(name)),
    channelMap(CHANNEL_MAP_SIZE) {

  SC_METHOD(sendLoop);
  sensitive << dataToSendEvent;
  dont_initialize();

  SC_METHOD(receivedCredit);
  sensitive << validCredit.pos();
  dont_initialize();

}
