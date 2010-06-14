/*
 * FetchLogic.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "FetchLogic.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../Cluster.h"

/* When a request for an instruction is received, see if it is in the cache. */
void FetchLogic::doOp() {
  // Save the address so it can be updated with the base address arrives
  offsetAddr = in.read();
  awaitingBaseAddr = true;
}

/* Add the base address onto our offset address, but only if we're expecting it. */
void FetchLogic::haveBaseAddress() {

  if(awaitingBaseAddr) {
    int base = baseAddress.read().getData();
    offsetAddr.addOffset(base);

    // Requires that base address arrives last. Bad?
    toIPKC.write(offsetAddr);
    awaitingBaseAddr = false;
  }

}

/* If the cache doesn't contain the desired Instruction, send a request to
 * the memory. Otherwise, we don't have to do anything. */
void FetchLogic::haveResultFromCache() {

  // Create a new memory request and wrap it up in an AddressedWord
  MemoryRequest mr(offsetAddr.getAddress(), MemoryRequest::IPK_READ);
  AddressedWord request(mr, offsetAddr.getChannelID());

  if(!(cacheContainsInst.read())) {
    toSend.write(request);     // Put the new request in the queue
    wake(sendData);            // Signal that there is new data to send

    // Stall so we don't receive any more data if the buffer is full
    if(toSend.isFull()) {
      stallValue = true;
      wake(stall);
    }
  }
  else {
    // Store the request in case the packet gets overwritten and needs to
    // be refetched.
    refetchRequest = request;
  }

}

void FetchLogic::doRefetch() {
  if(DEBUG) cout << this->name() << " performing refetch." << endl;

  toSend.write(refetchRequest);
  wake(sendData);

  // Stall so we don't receive any more requests if the buffer is full.
  if(toSend.isFull()) {
    stallValue = true;
    wake(stall);
  }
}

/* The flowControl signal tells us that we are now free to use the channel
 * again. Alternatively, the cache tells us that there is now room for any
 * instruction packet. */
void FetchLogic::sendNext() {
  if(DEBUG && isRoomToFetch.event())
    cout << this->name() << ": Now room in cache." << endl;

  if(toSend.isEmpty()) return;
  else if(flowControl.read() && isRoomToFetch.read()) {
    toNetwork.write(toSend.read());
    stallOut.write(false);  // Buffer is no longer full - can receive new data

    if(DEBUG) cout << this->name() << " sending fetch." << endl;
  }
  else {
    if(DEBUG) cout << "Not able to send FETCH from FetchLogic: "
        << (flowControl.read() ? "" : "flow control ")
        << (isRoomToFetch.read() ? "" : "no room") << endl;
  }
}

void FetchLogic::updateStall() {
  stallOut.write(stallValue);
}

FetchLogic::FetchLogic(sc_module_name name, int ID) :
    Component(name),
    toSend(4) {           // Can have 4 outstanding fetches (make a parameter?)

  id = ID;
  awaitingBaseAddr = false;

// Register methods
  SC_METHOD(doOp);
  sensitive << in;
  dont_initialize();

  SC_METHOD(haveResultFromCache);
  sensitive << cacheContainsInst;
  dont_initialize();

  SC_METHOD(haveBaseAddress);
  sensitive << baseAddress;
  dont_initialize();

  SC_METHOD(sendNext);
  sensitive << flowControl.pos() << sendData << isRoomToFetch.pos();
  dont_initialize();

  SC_METHOD(doRefetch);
  sensitive << refetch;
  dont_initialize();

  SC_METHOD(updateStall);
  sensitive << stall;
  dont_initialize();
}

FetchLogic::~FetchLogic() {

}
