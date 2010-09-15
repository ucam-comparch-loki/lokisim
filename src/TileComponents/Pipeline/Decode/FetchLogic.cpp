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

void FetchLogic::fetch(Address addr) {
  // Create a new memory request and wrap it up in an AddressedWord
  MemoryRequest mr(addr.getAddress(), MemoryRequest::IPK_READ);
  AddressedWord request(mr, addr.getChannelID());

  if(!inCache(addr)) {
    toSend.write(request);     // Put the new request in the queue
  }
  else {
    // Store the request in case the packet gets overwritten and needs to
    // be refetched.
    refetchRequest = request;
  }
}

void FetchLogic::refetch() {
  // What happens if toSend is full? Is it safe to assume that we will never
  // write to a full buffer?
  toSend.write(refetchRequest);
}

void FetchLogic::send() {
  if(!toSend.isEmpty() && flowControl.read() && roomInCache()) {
    toNetwork.write(toSend.read());
    if(DEBUG) cout << this->name() << " sending fetch." << endl;
  }
}

bool FetchLogic::inCache(Address addr) {
  return parent()->inCache(addr);
}

bool FetchLogic::roomInCache() {
  return parent()->roomToFetch();
}

DecodeStage* FetchLogic::parent() const {
  return (DecodeStage*)(this->get_parent());
}

FetchLogic::FetchLogic(sc_module_name name, int ID) :
    Component(name),
    toSend(4) {           // Can have 4 outstanding fetches (make a parameter?)

  id = ID;

}

FetchLogic::~FetchLogic() {

}
