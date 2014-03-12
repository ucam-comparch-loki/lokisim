/*
 * IndirectRegisterFile.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "RegisterFile.h"
#include "../Core.h"
#include "../../Datatype/Word.h"
#include "../../Utility/Instrumentation.h"
#include "../../Utility/Instrumentation/Registers.h"

const int32_t RegisterFile::read(PortIndex port, RegisterIndex reg, bool indirect) const {
  RegisterIndex index;

  if (indirect) {
    if (isChannelEnd(reg)) index = parent()->readRCET(toChannelID(reg));
    else                   index = regs.read(reg).toInt();
  }
  else index = reg;

  // What if indirect reading results in accessing the receive channel-end table
  // twice in one cycle? That probably wouldn't fit in one cycle.
  //  1. Get RCET to notice this and add a delay.
  //  2. Disallow this at software level.

  // If the indirect address points to a channel-end, read from there instead
  if (isChannelEnd(index)) {
    // Do we want to allow indirecting into the channel end table? I think
    // we need it to make the selch instruction useful.
    return parent()->readRCET(toChannelID(index));
  }
  else {
    int data = regs.read(index).toInt();

    if (ENERGY_TRACE) {
      Instrumentation::Registers::read(port, reg, prevRead[port], data);
      const_cast<RegisterFile*>(this)->logActivity(); // Hack
    }

    if (DEBUG) cout << this->name() << ": Read " << data
                    << " from register " << (int)index << endl;

    // A bit of a hack to allow us to store debug information from inside a
    // const method.
    const_cast<RegisterFile*>(this)->prevRead[port] = data;
    return data;
  }
}

const int32_t RegisterFile::readDebug(const RegisterIndex reg) const {
  return regs.read(reg).toInt();
}

void RegisterFile::write(const RegisterIndex reg, const int32_t value, bool indirect) {

  RegisterIndex index = indirect ? regs.read(reg).toInt() : reg;

  // There are some registers that we can't write to.
  if (isReserved(index)/* || isChannelEnd(index)*/) {
    if (index != 0) cerr << "Warning: attempting to write to reserved register "
                         << (int)index << endl;
    return;
  }

  int oldData = regs.read(index).toInt();

  Word w(value);
  writeReg(index, w);

  if (ENERGY_TRACE) {
    Instrumentation::Registers::write(index, oldData, value);
    logActivity();
  }

}

/* Register 0 is reserved to hold the constant value 0.
 * Register 1 is reserved to hold the address of the currently executing
 * instruction packet. */
bool RegisterFile::isReserved(RegisterIndex position) {
  return position >= 0
      && position <  2;
}

bool RegisterFile::isChannelEnd(RegisterIndex position) {
  return position >= START_OF_INPUT_CHANNELS
      && position <  START_OF_INPUT_CHANNELS + NUM_RECEIVE_CHANNELS;
}

bool RegisterFile::isAddressableReg(RegisterIndex position) {
  return !(isReserved(position) || isChannelEnd(position))
      && position < NUM_ADDRESSABLE_REGISTERS;
}

bool RegisterFile::needsIndirect(RegisterIndex position) {
  return position >= NUM_ADDRESSABLE_REGISTERS
      && position <  NUM_PHYSICAL_REGISTERS;
}

bool RegisterFile::isInvalid(RegisterIndex position) {
  return position < 0
      || position > NUM_PHYSICAL_REGISTERS;
}

RegisterIndex RegisterFile::toChannelID(RegisterIndex position) {
  assert(isChannelEnd(position));
  return position - START_OF_INPUT_CHANNELS;
}

RegisterIndex RegisterFile::fromChannelID(RegisterIndex position) {
  assert(position < NUM_RECEIVE_CHANNELS);
  return position + START_OF_INPUT_CHANNELS;
}

void RegisterFile::updateCurrentIPK(const MemoryAddr addr) {
  Word w(addr);
  writeReg(1, w);
}

void RegisterFile::writeReg(const RegisterIndex reg, const Word data) {
  if (DEBUG) cout << this->name() << ": Stored " << data << " to register " <<
      (int)reg << endl;

  regs.write(data, reg);
}

void RegisterFile::logActivity() {
  cycle_count_t currentCycle = Instrumentation::currentCycle();
  if (currentCycle != lastActivity) {
    Instrumentation::Registers::activity();
    lastActivity = currentCycle;
  }
}

Core* RegisterFile::parent() const {
  return static_cast<Core*>(this->get_parent());
}

RegisterFile::RegisterFile(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    regs(NUM_PHYSICAL_REGISTERS, std::string(name)),
    prevRead(3),
    lastActivity(-1) {

}