/*
 * IndirectRegisterFile.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "RegisterFile.h"

#include "../../Datatype/Word.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation.h"
#include "../../Utility/Instrumentation/Registers.h"
#include "../../Exceptions/InvalidOptionException.h"
#include "Core.h"

int32_t RegisterFile::read(PortIndex port, RegisterIndex reg, bool indirect) const {
  RegisterIndex index;

  if (indirect) {
    if (isChannelEnd(reg)) index = parent()->readRCET(toChannelID(reg));
    else                   index = readInternal(reg);
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
    int data = readInternal(reg);

    if (ENERGY_TRACE) {
      Instrumentation::Registers::read(port, reg, prevRead[port], data);
      const_cast<RegisterFile*>(this)->logActivity(); // Hack
    }

    LOKI_LOG << this->name() << ": Read " << data
             << " from register " << (int)index << endl;

    // A bit of a hack to allow us to store debug information from inside a
    // const method.
    const_cast<RegisterFile*>(this)->prevRead[port] = data;
    return data;
  }
}

int32_t RegisterFile::readInternal(const RegisterIndex reg) const {
  if (reg == 0)
    return 0;
  else
    return regs.read(reg).toInt();
}

void RegisterFile::write(const RegisterIndex reg, int32_t value, bool indirect) {

  RegisterIndex index = indirect ? regs.read(reg).toInt() : reg;

  // There are some registers that we can't write to.
  if (isReserved(index)/* || isChannelEnd(index)*/ && index != 0)
    throw InvalidOptionException("destination register", index);

  int oldData = regs.read(index).toInt();

  Word w(value);
  writeInternal(index, w);

  if (ENERGY_TRACE) {
    Instrumentation::Registers::write(index, oldData, value);
    logActivity();
  }

}

/* Register 0 is reserved to hold the constant value 0.
 * Register 1 is reserved to hold the address of the currently executing
 * instruction packet. */
bool RegisterFile::isReserved(RegisterIndex position) {
  return position <  2;
}

bool RegisterFile::isChannelEnd(RegisterIndex position) {
  return position >= START_OF_INPUT_CHANNELS
      && position <  START_OF_INPUT_CHANNELS + CORE_RECEIVE_CHANNELS;
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
  return position > NUM_PHYSICAL_REGISTERS;
}

RegisterIndex RegisterFile::toChannelID(RegisterIndex position) {
  assert(isChannelEnd(position));
  return position - START_OF_INPUT_CHANNELS;
}

RegisterIndex RegisterFile::fromChannelID(RegisterIndex position) {
  assert(position < CORE_RECEIVE_CHANNELS);
  return position + START_OF_INPUT_CHANNELS;
}

void RegisterFile::updateCurrentIPK(MemoryAddr addr) {
  Word w(addr);
  writeInternal(1, w);
}

void RegisterFile::writeInternal(RegisterIndex reg, const Word data) {
  if (reg > 0)
    LOKI_LOG << this->name() << ": Stored " << data << " to register " << (int)reg << endl;

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
  return static_cast<Core*>(this->get_parent_object());
}

RegisterFile::RegisterFile(sc_module_name name, const ComponentID& ID) :
    LokiComponent(name, ID),
    regs(NUM_PHYSICAL_REGISTERS, std::string(name)),
    prevRead(3),
    lastActivity(-1) {

}
