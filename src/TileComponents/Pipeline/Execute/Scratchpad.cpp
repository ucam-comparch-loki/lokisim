/*
 * Scratchpad.cpp
 *
 *  Created on: 17 Jan 2013
 *      Author: db434
 */

#include "Scratchpad.h"
#include "../../../Utility/Instrumentation/Scratchpad.h"

Scratchpad::Scratchpad(const sc_module_name& name, const ComponentID& ID) :
    Component(name, ID),
    data(CORE_SCRATCHPAD_SIZE, std::string(name)) {
  // Do nothing.
}

const int32_t Scratchpad::read(const MemoryAddr addr) const {
  Instrumentation::Scratchpad::read();
  int32_t result = data.read(addr).toInt();

  if (DEBUG)
    cout << this->name() << " read " << result << " from position " << addr << endl;

  return result;
}

void Scratchpad::write(const MemoryAddr addr, const int32_t value) {
  Instrumentation::Scratchpad::write();
  data.write(value, addr);

  if (DEBUG)
    cout << this->name() << " wrote " << value << " to position " << addr << endl;
}
