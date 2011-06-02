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

const int32_t IndirectRegisterFile::read(const RegisterIndex reg, bool indirect) const {
  RegisterIndex index = indirect ? indirectRegs.read(reg) : reg;

  // If the indirect address points to a channel-end, read from there instead
  if(isChannelEnd(index)) {
    // Do we want to allow indirecting into the channel end table? I think
    // we need it to make the selch instruction useful.
    return parent()->readRCET(toChannelID(index));
  }
  else {
    Instrumentation::registerRead(id, index);
    if(DEBUG) cout << this->name() << ": Read " << regs.read(index)
                   << " from register " << (int)index << endl;
    return regs.read(index).toInt();
  }
}

const int32_t IndirectRegisterFile::readDebug(const RegisterIndex reg) const {
  return regs.read(reg).toInt();
}

void IndirectRegisterFile::write(const RegisterIndex reg, const int32_t value, bool indirect) {

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

  Instrumentation::registerWrite(id, index);

}

/* Store a subsection of the data into the indirect register at position
 * "address". Since NUM_PHYSICAL_REGISTERS registers must be accessible, the
 * indirect registers must hold ceil(log2(NUM_PHYSICAL_REGISTERS)) bits each. */
void IndirectRegisterFile::updateIndirectReg(const RegisterIndex address, const Word data) {

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

void IndirectRegisterFile::updateCurrentIPK(const MemoryAddr addr) {
  Word w(addr);
  writeReg(1, w);
}

void IndirectRegisterFile::writeReg(const RegisterIndex reg, const Word data) {
  regs.write(data, reg);

  if(DEBUG) cout << this->name() << ": Stored " << data << " to register " <<
      (int)reg << endl;

  // Instrumentation...
}

Cluster* IndirectRegisterFile::parent() const {
  return (Cluster*)(this->get_parent());
}

IndirectRegisterFile::IndirectRegisterFile(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    regs(NUM_PHYSICAL_REGISTERS, std::string(name)),
    indirectRegs(NUM_ADDRESSABLE_REGISTERS, std::string(name)) {

}
