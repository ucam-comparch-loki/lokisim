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

void FetchLogic::fetch(const MemoryAddr addr) {
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

	// Send request if necessary or store it for later use

	AddressedWord request(mr, channel);

	if(!inCache(addr)) {
		sendRequest(request);
	} else {
		// Store the request in case the packet gets overwritten and needs to
		// be refetched.

		refetchRequest = request;
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

void FetchLogic::refetch() {
  sendRequest(refetchRequest);
}

void FetchLogic::sendRequest(const AddressedWord& data) {
  dataToSend = data;
  sendEvent.notify();
}

void FetchLogic::send() {
  while(true) {
    wait(sendEvent);

    // FIXME: whilst waiting for flow control and cache, etc. we may receive
    // another request. One will be lost.
    while(!flowControl.read() && !roomInCache()) {
      wait(1, sc_core::SC_NS);
    }
    toNetwork.write(dataToSend);
  }
}

bool FetchLogic::inCache(const MemoryAddr addr) const {
  return parent()->inCache(addr);
}

bool FetchLogic::roomInCache() const {
  return parent()->roomToFetch();
}

DecodeStage* FetchLogic::parent() const {
  // Need a dynamic cast because DecodeStage uses virtual inheritance.
  return dynamic_cast<DecodeStage*>(this->get_parent());
}

FetchLogic::FetchLogic(sc_module_name name, const ComponentID& ID) :
    Component(name, ID) {

  fetchChannel = -1;      // So we get warnings if we fetch before setting this.

  SC_THREAD(send);

}
