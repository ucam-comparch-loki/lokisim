/*
 * FetchLogic.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "FetchLogic.h"

/* When a request for an instruction is received, see if it is in the cache. */
void FetchLogic::doOp() {
  std::cout << "Received address: " << in.read() << std::endl;
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
    std::cout << "Sent address: " << offsetAddr << std::endl;
  }

}

/* If the cache doesn't contain the desired Instruction, send a request to
 * the memory. */
void FetchLogic::haveResultFromCache() {

  if(!(cacheContainsInst.read())) {

    // Send return address (our channel 1) to memory
    Address *myAddress = new Address(id, 1);
    AddressedWord *retAddr = new AddressedWord(*myAddress, offsetAddr.getChannelID());

    // Send address to read from to memory
    AddressedWord *sendAddr = new AddressedWord(offsetAddr, offsetAddr.getChannelID());

    toSend.write(*retAddr);
    toSend.write(*sendAddr);
    if(canSend && isRoomToFetch.read()) sendData.write(!sendData.read());
    else std::cout << "Not able to send FETCH from FetchLogic." << std::endl;
  }

}

/* The flowControl signal tells us that we are now free to use the channel again. */
void FetchLogic::sendNext() {
  if(toSend.isEmpty()) canSend = true;
  else if(isRoomToFetch.read()) {
    toNetwork.write(toSend.read());
    canSend = false;
  }
}

FetchLogic::FetchLogic(sc_core::sc_module_name name, int ID) :
    Component(name, ID),
    toSend(8) {           // Can have 4 outstanding fetches (2 flits each)

  canSend = true;
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
  sensitive << flowControl  << sendData << isRoomToFetch;
  dont_initialize();

}

FetchLogic::~FetchLogic() {

}
