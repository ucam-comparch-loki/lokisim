/*
 * IndirectRegisterFile.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include <math.h>
#include "IndirectRegisterFile.h"
#include "../Cluster.h"
#include "../../Datatype/Word.h"

int32_t IndirectRegisterFile::read(RegisterIndex reg, bool indirect) const {
  RegisterIndex index = indirect ? indirectRegs.read(reg) : reg;

  // If the indirect address points to a channel-end, read from there instead
  if(isChannelEnd(index)) {
    // Do we want to allow indirecting into the channel end table? I think
    // we need it to make the selch instruction useful.
    return parent()->readRCET(toChannelID(index));
  }
  else {
    if(DEBUG) cout << this->name() << ": Read " << regs.read(index)
                   << " from register " << (int)index << endl;
    return regs.read(index).toInt();
  }
}

int32_t IndirectRegisterFile::readDebug(RegisterIndex reg) const {
  return regs.read(reg).toInt();
}

void IndirectRegisterFile::write(RegisterIndex reg, int32_t value, bool indirect) {

  RegisterIndex index = indirect ? indirectRegs.read(reg) : reg;

  // There are some registers that we can't write to.
  if(isReserved(index)/* || isChannelEnd(index)*/) {
    if(index != 0) cerr << "Warning: attempting to write to reserved register "
                        << (int)index << endl;
    return;
  }

  Word w(value);
  writeReg(index, w);
  updateIndirectReg(index, w);

}

/* Store a subsection of the data into the indirect register at position
 * "address". Since NUM_PHYSICAL_REGISTERS registers must be accessible, the
 * indirect registers must hold ceil(log2(NUM_PHYSICAL_REGISTERS)) bits each. */
void IndirectRegisterFile::updateIndirectReg(RegisterIndex address, Word data) {

  // Don't store anything if there is no indirect register to store to.
  if(address >= NUM_ADDRESSABLE_REGISTERS) return;

  static int numBits = ceil(log2(NUM_PHYSICAL_REGISTERS));

  uint8_t lowestBits = data.lowestBits(numBits);
  indirectRegs.write(lowestBits, address);

}

/* Register 0 is reserved to hold the constant value 0.
 * Register 1 is reserved to hold the address of the currently executing
 * instruction packet. */
bool IndirectRegisterFile::isReserved(RegisterIndex position) {
  return position >= 0
      && position <  2;
}

bool IndirectRegisterFile::isChannelEnd(RegisterIndex position) {
  return position >= START_OF_INPUT_CHANNELS
      && position <  START_OF_INPUT_CHANNELS + NUM_RECEIVE_CHANNELS;
}

bool IndirectRegisterFile::isAddressableReg(RegisterIndex position) {
  return !(isReserved(position) || isChannelEnd(position))
      && position < NUM_ADDRESSABLE_REGISTERS;
}

bool IndirectRegisterFile::needsIndirect(RegisterIndex position) {
  return position >= NUM_ADDRESSABLE_REGISTERS
      && position <  NUM_PHYSICAL_REGISTERS;
}

bool IndirectRegisterFile::isInvalid(RegisterIndex position) {
  return position < 0
      || position > NUM_PHYSICAL_REGISTERS;
}

RegisterIndex IndirectRegisterFile::toChannelID(RegisterIndex position) {
  // Check that it is in fact a channel-end?
  return position - START_OF_INPUT_CHANNELS;
}

RegisterIndex IndirectRegisterFile::fromChannelID(RegisterIndex position) {
  // Check that it is in fact a channel-end?
  return position + START_OF_INPUT_CHANNELS;
}

void IndirectRegisterFile::updateCurrentIPK(Address addr) {
  // setfetchch specifies the channel, so only the address is required here.
  Word w(addr.address());
  writeReg(1, w);
}

void IndirectRegisterFile::writeReg(RegisterIndex reg, Word data) {
  regs.write(data, reg);

  if(DEBUG) cout << this->name() << ": Stored " << data << " to register " <<
      (int)reg << endl;

  // Instrumentation...
}

Cluster* IndirectRegisterFile::parent() const {
  return (Cluster*)(this->get_parent());
}

IndirectRegisterFile::IndirectRegisterFile(sc_module_name name) :
    Component(name),
    regs(NUM_PHYSICAL_REGISTERS, string(name)),
    indirectRegs(NUM_ADDRESSABLE_REGISTERS, string(name)) {

}

IndirectRegisterFile::~IndirectRegisterFile() {

}
