/*
 * FetchLogic.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "FetchLogic.h"
#include "DecodeStage.h"
#include "../../TileComponent.h"
#include "../../../Datatype/MemoryRequest.h"

void FetchLogic::fetch(const MemoryAddr addr, bool checkedCache) {
	// Create a new memory request

	MemoryRequest mr(MemoryRequest::IPK_READ, addr);

	// Adjust fetch channel based on memory configuration if necessary

	ChannelID channel = fetchChannel;

	if (fetchChannel.isMemory() && memoryGroupBits_ > 0) {
		uint32_t increment = (uint32_t)addr;
		increment &= (1UL << (memoryGroupBits_ + memoryLineBits_)) - 1UL;
		increment >>= memoryLineBits_;
		channel = channel.addPosition(increment);
	}

	// If we know that the packet isn't in the cache, send a request to memory.
  if(checkedCache || !inCache(addr)) {
    AddressedWord request(mr, channel);
    sendRequest(request);
  }
}

void FetchLogic::setFetchChannel(const ChannelID& channelID, uint memoryGroupBits, uint memoryLineBits) {
	fetchChannel = channelID;
	memoryGroupBits_ = memoryGroupBits;
	memoryLineBits_ = memoryLineBits;

	ChannelID returnChannel(id, 0);

	if (fetchChannel.isCore()) {
		// Destination is a core

		// Need to claim this port so that it sends flow control information back
		// here.

		AddressedWord aw(Word(returnChannel.getData()), fetchChannel);
		aw.setPortClaim(true, true);

		// Not ideal: the send method waits for there to be room in the cache, but
		// for just claiming a port, this isn't necessary.

		sendRequest(aw);
	} else {
		// Destination is a memory

		/*
		// Send memory channel table setup request

		MemoryRequest mr;
		mr.setHeaderSetTableEntry(returnChannel.getData());
		AddressedWord aw(mr, fetchChannel);
		sendRequest(aw);
		*/
	}
}

void FetchLogic::sendRequest(const AddressedWord& data) {
  dataToSend = data;
  sendEvent.notify();
}

void FetchLogic::send() {
	// Assert dataToSend is new/valid.

  // Wait until flow control allows us to send and there is room in the cache.
  if(!flowControl.read() || !roomInCache()) {
    next_trigger(clock.posedge_event());
  }
  else {
    toNetwork.write(dataToSend);
    next_trigger(sendEvent);
  }
}

bool FetchLogic::inCache(const MemoryAddr addr) const {
  return parent()->inCache(addr);
}

bool FetchLogic::roomInCache() const {
  return parent()->roomToFetch();
}

DecodeStage* FetchLogic::parent() const {
  return static_cast<DecodeStage*>(this->get_parent());
}

FetchLogic::FetchLogic(sc_module_name name, const ComponentID& ID) :
    Component(name, ID) {

  fetchChannel = -1;      // So we get warnings if we fetch before setting this.

  SC_METHOD(send);
  sensitive << sendEvent;
  dont_initialize();

}
