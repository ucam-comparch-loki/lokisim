/*
 * OffChip.cpp
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#include "OffChip.h"
#include "../Datatype/AddressedWord.h"

void OffChip::newData() {
  // Ignore the data and send a credit, so the sender doesn't block.
  cerr << "Warning: Offchip component received data: " << dataIn.read() << endl;
}

OffChip::OffChip(sc_module_name name) : Component(name) {
  SC_METHOD(newData);
  sensitive << dataIn;
  dont_initialize();

  readyOut.initialize(true);
}

OffChip::~OffChip() {

}
