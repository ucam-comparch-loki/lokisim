/*
 * MemoryMat.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "MemoryMat.h"

/* Determine whether to read or write. */
void MemoryMat::doOp1() {
  // Cast the Word to an Address to interpret it correctly
  if((static_cast<Address>(in3.read())).getReadBit()) read1();
  else write1();
}

void MemoryMat::doOp2() {
  // Cast the Word to an Address to interpret it correctly
  if((static_cast<Address>(in4.read())).getReadBit()) read2();
  else write2();
}

/* Read from memory and send the result to the given address. */
void MemoryMat::read1() {
  // Cast the Word to an Address to interpret it correctly
  int readAddress = (static_cast<Address>(in1.read())).getAddress();
  Word inMem = data.read(readAddress);
  Word copy = inMem;

  int sendAddress = (static_cast<Address>(in3.read())).getAddress();
  AddressedWord *aw = new AddressedWord(copy, 0, sendAddress);

  Array<AddressedWord> *toSend = new Array<AddressedWord>(1);
  toSend->put(0, *aw);
  out.write(*toSend);    // Need to make sure writes aren't conflicting
}

void MemoryMat::read2() {
  // Cast the Word to an Address to interpret it correctly
  int readAddress = (static_cast<Address>(in2.read())).getAddress();
  Word inMem = data.read(readAddress);
  Word copy = inMem;

  int sendAddress = (static_cast<Address>(in4.read())).getAddress();
  AddressedWord *aw = new AddressedWord(copy, 0, sendAddress);

  Array<AddressedWord> *toSend = new Array<AddressedWord>(1);
  toSend->put(0, *aw);
  out.write(*toSend);    // Need to make sure writes aren't conflicting
}

/* Write the given data into the given memory address. */
void MemoryMat::write1() {
  // Cast the Word to an Address to interpret it correctly
  int writeAddress = (static_cast<Address>(in3.read())).getAddress();
  Word newData = in1.read();
  data.write(newData, writeAddress);
}

void MemoryMat::write2() {
  // Cast the Word to an Address to interpret it correctly
  int writeAddress = (static_cast<Address>(in4.read())).getAddress();
  Word newData = in2.read();
  data.write(newData, writeAddress);
}

MemoryMat::MemoryMat(sc_core::sc_module_name name, int ID) :
    TileComponent(name, ID),
    data(MEMORY_SIZE) {

// Register methods
  SC_METHOD(doOp1);
  sensitive << in1;   // Execute doOp1 whenever data is received on in1

  SC_METHOD(doOp2);
  sensitive << in2;   // Execute doOp2 whenever data is received on in2
}

MemoryMat::~MemoryMat() {

}
