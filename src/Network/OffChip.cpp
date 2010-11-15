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
  creditsOut.write(1);
}

OffChip::OffChip(sc_module_name name) : Component(name) {
  SC_METHOD(newData);
  sensitive << dataIn;
  dont_initialize();
}

OffChip::~OffChip() {

}
