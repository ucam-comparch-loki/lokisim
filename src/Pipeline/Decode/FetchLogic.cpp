/*
 * FetchLogic.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "FetchLogic.h"

/* When a request for an instruction is received, see if it is in the cache. */
void FetchLogic::doOp() {

  // Check the cache to see if we already have this Instruction
  toIPKC.write(in.read());

}

/* If the cache doesn't contain the desired Instruction, send a request to
 * the memory. */
void FetchLogic::haveResultFromCache() {
  if(!(cacheContainsInst.read())) {
    // Probably shouldn't send out two items at once

    // Send address to read from to memory's channel 0 or 1
    AddressedWord *sendAddr = new AddressedWord(in.read(), in.read().getAddress(), 0);
    toNetwork.write(*sendAddr);

    // Wait 1 cycle?

    // Send return address (our channel 1) to memory's channel 2 or 3
    Address *myAddress = new Address(id, 1);
    AddressedWord *retAddr = new AddressedWord(*myAddress, in.read().getAddress(), 2);
    toNetwork.write(*retAddr);

    // Deal with flow control?
  }
  // Need to make sure request is reset to 0 when not requesting
  //else toNetwork.write(NULL);
}

FetchLogic::FetchLogic(sc_core::sc_module_name name, int ID)
    : Component(name, ID) {

  SC_METHOD(doOp);
  sensitive << in;

  SC_METHOD(haveResultFromCache);
  sensitive << cacheContainsInst;

}

FetchLogic::~FetchLogic() {

}
