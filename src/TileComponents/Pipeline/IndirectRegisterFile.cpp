/*
 * IndirectRegisterFile.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "IndirectRegisterFile.h"
#include <math.h>

int32_t IndirectRegisterFile::read(uint8_t reg, bool indirect) const {
  uint8_t index = indirect ? indirectRegs.read(reg) : reg;

  // If the indirect address points to a channel-end, read from there instead
//  if(isChannelEnd(index)) {
//    return rcet.read(toChannelID(index));
//  }
  /*else*/ {
    if(DEBUG) cout << this->name() << ": Read " << regs.read(index)
                   << " from register " << index << endl;
    return regs.read(index).toInt();
  }
}

void IndirectRegisterFile::write(uint8_t reg, int32_t value, bool indirect) {

  uint8_t index = indirect ? indirectRegs.read(reg) : reg;

  // There are some registers that we can't write to.
  if(isReserved(index)/* || isChannelEnd(index)*/) return;

  Word w(value);
  regs.write(w, index);
  updateIndirectReg(index, w);

  if(DEBUG) cout<<this->name()<<": Stored "<<w<<" to register "<<index<<endl;

}

/* Store a subsection of the data into the indirect register at position
 * "address". Since NUM_PHYSICAL_REGISTERS registers must be accessible, the
 * indirect registers must hold ceil(log2(NUM_PHYSICAL_REGISTERS)) bits each. */
void IndirectRegisterFile::updateIndirectReg(int address, Word data) {

  // Don't store anything if there is no indirect register to store to.
  if(address >= NUM_ADDRESSABLE_REGISTERS) return;

  static int numBits = ceil(log2(NUM_PHYSICAL_REGISTERS));

  uint8_t lowestBits = (static_cast<Address>(data)).getLowestBits(numBits);
  indirectRegs.write(lowestBits, address);

}

/* Register 0 is reserved to hold the constant value 0.
 * Register 1 is reserved to hold the address of the currently executing
 * instruction packet. */
bool IndirectRegisterFile::isReserved(int position) {
  return position >= 0
      && position <  2;
}

bool IndirectRegisterFile::isChannelEnd(int position) {
  return position >= START_OF_INPUT_CHANNELS
      && position <  START_OF_INPUT_CHANNELS + NUM_RECEIVE_CHANNELS;
}

bool IndirectRegisterFile::isAddressableReg(int position) {
  return !(isReserved(position) || isChannelEnd(position))
      && position < NUM_ADDRESSABLE_REGISTERS;
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
  return position - START_OF_INPUT_CHANNELS;
}

int IndirectRegisterFile::fromChannelID(int position) {
  // Check that it is in fact a channel-end?
  return position + START_OF_INPUT_CHANNELS;
}

void IndirectRegisterFile::updateCurrentIPK(Address addr) {
  // setfetchch specifies the channel, so only the address is required here.
  Word w(addr.getAddress());
  regs.write(w, 1);
}

IndirectRegisterFile::IndirectRegisterFile(sc_module_name name) :
    Component(name),
    regs(NUM_PHYSICAL_REGISTERS),
    indirectRegs(NUM_ADDRESSABLE_REGISTERS) {

}

IndirectRegisterFile::~IndirectRegisterFile() {

}
