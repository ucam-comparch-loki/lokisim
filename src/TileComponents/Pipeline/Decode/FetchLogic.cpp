/*
 * FetchLogic.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "FetchLogic.h"
#include "DecodeStage.h"
#include "../../../Datatype/Address.h"
#include "../../../Datatype/MemoryRequest.h"

void FetchLogic::fetch(uint16_t addr) {
  // Create a new memory request and wrap it up in an AddressedWord
  MemoryRequest mr(addr, MemoryRequest::IPK_READ);
  AddressedWord request(mr, fetchChannel);

  if(!inCache(Address(addr, fetchChannel))) {
    toSend.write(request);     // Put the new request in the queue
  }
  else {
    // Store the request in case the packet gets overwritten and needs to
    // be refetched.
    refetchRequest = request;
  }
}

void FetchLogic::setFetchChannel(uint16_t channelID) {
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
  if(!toSend.isEmpty() && flowControl.read()) {
    if(toSend.peek().portClaim()) {
      toNetwork.write(toSend.read());
    }
    else if(roomInCache()) {
      toNetwork.write(toSend.read());
      if(DEBUG) cout << this->name() << " sending fetch: " << toNetwork.read()
                     << "." << endl;
    }
  }
}

bool FetchLogic::inCache(Address addr) {
  return parent()->inCache(addr);
}

bool FetchLogic::roomInCache() {
  return parent()->roomToFetch();
}

uint16_t FetchLogic::portID() {
  return id*NUM_CLUSTER_OUTPUTS;
}

DecodeStage* FetchLogic::parent() const {
  return (DecodeStage*)(this->get_parent());
}

FetchLogic::FetchLogic(sc_module_name name, int ID) :
    Component(name),
    toSend(4) {           // Can have 4 outstanding fetches (make a parameter?)

  id = ID;
  fetchChannel = -1;      // So we get warnings if we fetch before setting this.

}

FetchLogic::~FetchLogic() {

}
