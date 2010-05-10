/*
 * IndirectRegisterFile.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "IndirectRegisterFile.h"
#include <math.h>


/* Read from the address given in readAddr1. */
void IndirectRegisterFile::read1() {
  short addr = readAddr1.read();
  out1.write(regs.read(addr));

  if(DEBUG) cout << this->name() << ": Read " << regs.read(addr) <<
                    " from register " << addr << endl;
}

/* Read from the address given (or pointed to) by readAddr2. */
void IndirectRegisterFile::read2() {
  short addr = readAddr2.read();

  if(indRead.read()) {
    addr = indirect.read(addr);

    // If the indirect address points to a channel-end, read from there instead
    if(isChannelEnd(addr)) {
      channelID.write(toChannelID(addr));
      return;
    }
  }

  out2.write(regs.read(addr));

  if(DEBUG) cout << this->name() << ": Read " << regs.read(addr) <<
                    " from register " << addr << endl;
}

/* Write to the address given in the register pointed to by indWriteAddr. */
void IndirectRegisterFile::indirectWrite() {
  short addr = indWriteAddr.read();
  addr = indirect.read(addr);           // Indirect

  // There are some registers that we can't write to.
  if(isReserved(addr) || isChannelEnd(addr)) return;

  Word w = writeData.read();
  regs.write(w, addr);

  updateIndirectReg(addr, writeData.read());
}

/* Write to the address given in writeAddr. */
void IndirectRegisterFile::write() {
  short addr = writeAddr.read();

  // There are some registers that we can't write to.
  if(isReserved(addr) || isChannelEnd(addr)) return;

  Word w = writeData.read();
  regs.write(w, addr);

  if(DEBUG) cout<<this->name()<<": Stored "<<w<<" to register "<<addr<<endl;

  updateIndirectReg(addr, writeData.read());
}

/* Store a subsection of the data into the indirect register at position
 * "address". Since NUM_PHYSICAL_REGISTERS registers must be accessible, the
 * indirect registers must hold ceil(log2(NUM_PHYSICAL_REGISTERS)) bits each. */
void IndirectRegisterFile::updateIndirectReg(int address, Word data) {

  // Don't store anything if there is no indirect register to store to.
  if(address >= NUM_ADDRESSABLE_REGISTERS) return;

  static int numBits = ceil(log2(NUM_PHYSICAL_REGISTERS));

  short lowestBits = (static_cast<Address>(data)).getLowestBits(numBits);
  indirect.write(lowestBits, address);

}

/* Register 0 is reserved to hold the constant value 0.
 * Register 1 is reserved to hold the address of the currently executing
 * instruction packet. */
bool IndirectRegisterFile::isReserved(int position) {
  return position >= 0
      && position <  2;
}

bool IndirectRegisterFile::isChannelEnd(int position) {
  return position >= 2
      && position <  2 + NUM_RECEIVE_CHANNELS;
}

bool IndirectRegisterFile::isAddressableReg(int position) {
  return position >= 2 + NUM_RECEIVE_CHANNELS
      && position <  NUM_ADDRESSABLE_REGISTERS;
}

bool IndirectRegisterFile::needsIndirect(int position) {
  return position >= NUM_ADDRESSABLE_REGISTERS
      && position <  NUM_PHYSICAL_REGISTERS;
}

bool IndirectRegisterFile::isInvalid(int position) {
  return position < 0
      || position > NUM_PHYSICAL_REGISTERS;
}

int IndirectRegisterFile::toChannelID(int position) {
  // Check that it is in fact a channel-end?

  // Since there are two reserved registers before the channel-ends start,
  // the address must have two subtracted from it.
  return position - 2;
}

int IndirectRegisterFile::fromChannelID(int position) {
  // Check that it is in fact a channel-end?

  // Since there are two reserved registers before the channel-ends start,
  // the address must have two added from it.
  return position + 2;
}

void IndirectRegisterFile::updateCurrentIPK(Address addr) {
  // setfetchch specifies the channel, so only the address is required here.
  Word w(addr.getAddress());
  regs.write(w, 1);
}

IndirectRegisterFile::IndirectRegisterFile(sc_module_name name) :
    Component(name),
    regs(NUM_PHYSICAL_REGISTERS),
    indirect(NUM_ADDRESSABLE_REGISTERS) {

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
