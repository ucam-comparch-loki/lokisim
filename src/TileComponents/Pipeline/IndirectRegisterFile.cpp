/*
 * IndirectRegisterFile.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "IndirectRegisterFile.h"


/* Read from the address given in readAddr1 */
void IndirectRegisterFile::read1() {
  short addr = readAddr1.read();
  out1.write(regs.read(addr));

  if(DEBUG) std::cout<<"Read "<<regs.read(addr)<<" from register "<<addr<<"\n";
}

/* Read from the address given (or pointed to) by readAddr2 */
void IndirectRegisterFile::read2() {
  short addr = readAddr2.read();

  if(indRead.read()) addr = indirect.read(addr);

  out2.write(regs.read(addr));

  if(DEBUG) std::cout<<"Read "<<regs.read(addr)<<" from register "<<addr<<"\n";
}

/* Write to the address given in the register pointed to by indWriteAddr */
void IndirectRegisterFile::indirectWrite() {
  short addr = indWriteAddr.read();
  addr = indirect.read(addr);           // Indirect
  Word w = writeData.read();
  regs.write(w, addr);

  // Store the lowest 5 (currently) bits of the data in the indirect register file
  short toIndirect = (static_cast<Address>(writeData.read())).getLowestBits(4);
  indirect.write(toIndirect, addr);
}

/* Write to the address given in writeAddr */
void IndirectRegisterFile::write() {
  short addr = writeAddr.read();
  Word w = writeData.read();
  regs.write(w, addr);

  if(DEBUG) std::cout<<"Stored "<<w<<" to register "<<addr<<"\n";

  // Store the lowest 5 (currently) bits of the data in the indirect register file
  short toIndirect = (static_cast<Address>(writeData.read())).getLowestBits(4);
  indirect.write(toIndirect, addr);
}

IndirectRegisterFile::IndirectRegisterFile(sc_core::sc_module_name name) :
    Component(name),
    regs(NUM_REGISTERS),
    indirect(NUM_REGISTERS) {

// Register methods
  SC_METHOD(read1);
  sensitive << readAddr1;
  dont_initialize();

  SC_METHOD(read2);
  sensitive << readAddr2;
  dont_initialize();

  SC_METHOD(indirectWrite);
  sensitive << indWriteAddr;
  dont_initialize();

  SC_METHOD(write);
  sensitive << writeAddr;
  dont_initialize();

}

IndirectRegisterFile::~IndirectRegisterFile() {

}
