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
  // Create a new memory request and wrap it up in an AddressedWord
  MemoryRequest mr(addr, MemoryRequest::IPK_READ);
  AddressedWord request(mr, fetchChannel);

  if(!inCache(addr)) {
    sendRequest(request);
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
  AddressedWord aw(Word(portID()), fetchChannel, true);

  // Not ideal: the send method waits for there to be room in the cache, but
  // for just claiming a port, this isn't necessary.
  sendRequest(aw);
}

void FetchLogic::refetch() {
  sendRequest(refetchRequest);
}

void FetchLogic::sendRequest(AddressedWord& data) {
  dataToSend = data;
  sendEvent.notify(sc_core::SC_ZERO_TIME);
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

ChannelID FetchLogic::portID() const {
  return TileComponent::outputChannelID(id, 0);
}

DecodeStage* FetchLogic::parent() const {
  // Need a dynamic cast because DecodeStage uses virtual inheritance.
  return dynamic_cast<DecodeStage*>(this->get_parent());
}

FetchLogic::FetchLogic(sc_module_name name, int ID) :
    Component(name) {

  id = ID;
  fetchChannel = -1;      // So we get warnings if we fetch before setting this.

  SC_THREAD(send);

}
