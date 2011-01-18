/*
 * FetchLogic.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "FetchLogic.h"
#include "DecodeStage.h"
#include "../../TileComponent.h"
#include "../../../Datatype/Address.h"
#include "../../../Datatype/MemoryRequest.h"

void FetchLogic::fetch(const MemoryAddr addr) {
  // Create a new memory request and wrap it up in an AddressedWord
  MemoryRequest mr(addr, MemoryRequest::IPK_READ);
  AddressedWord request(mr, fetchChannel);

  // Play with fetchChannel so we have the component ID, rather than the port
  // number.
  if(!inCache(Address(addr, fetchChannel/NUM_CORE_INPUTS))) {
    toSend.write(request);     // Put the new request in the queue
  }
  else {
    // Store the request in case the packet gets overwritten and needs to
    // be refetched.
    refetchRequest = request;
  }
}

void FetchLogic::setFetchChannel(const ChannelID channelID) {
  fetchChannel = channelID;

  // Need to claim this port so that it sends flow control information back
  // here.
  AddressedWord aw(Word(portID()), channelID, true);
  toSend.write(aw);

  // Block if the buffer is now full?
}

void FetchLogic::refetch() {
  // What happens if toSend is full? Is it safe to assume that we will never
  // write to a full buffer?
  toSend.write(refetchRequest);
}

void FetchLogic::send() {
  // Three conditions must be satisfied before we can send a fetch request:
  //  1. There must be something to send.
  //  2. Flow control must allow us to send.
  //  3. There must be enough room in the cache for a new packet.
  if(!toSend.empty() && flowControl.read()) {
    // Setting up a memory connection doesn't result in receiving a packet,
    // so it is safe to send even if there isn't space in the cache.
    if(toSend.peek().portClaim()) {
      toNetwork.write(toSend.read());
    }
    else if(roomInCache()) {
      AddressedWord aw = toSend.read();
      toNetwork.write(aw);
      if(DEBUG) cout << this->name() << " sending fetch: " << aw
                     << "." << endl;
    }
  }
}

bool FetchLogic::inCache(const Address addr) const {
  return parent()->inCache(addr);
}

bool FetchLogic::roomInCache() const {
  return parent()->roomToFetch();
}

ChannelID FetchLogic::portID() const {
  return TileComponent::outputPortID(id, 0);
}

DecodeStage* FetchLogic::parent() const {
  // Need a dynamic cast because DecodeStage uses virtual inheritance.
  return dynamic_cast<DecodeStage*>(this->get_parent());
}

FetchLogic::FetchLogic(sc_module_name name, int ID) :
    Component(name),
    toSend(4, std::string(name)) { // Can have 4 outstanding fetches (make a parameter?)

  id = ID;
  fetchChannel = -1;      // So we get warnings if we fetch before setting this.

}

FetchLogic::~FetchLogic() {

}
