/*
 * Scratchpad.cpp
 *
 *  Created on: 17 Jan 2013
 *      Author: db434
 */

#include "Scratchpad.h"
#include "../../../Utility/Instrumentation/Scratchpad.h"

Scratchpad::Scratchpad(const sc_module_name& name,
                       const scratchpad_parameters_t& params) :
    LokiComponent(name),
    data(std::string(name), params.size) {
  // Do nothing.
}

int32_t Scratchpad::read(const MemoryAddr addr) const {
  Instrumentation::Scratchpad::read();
  int32_t result = data.read(addr).toInt();

  LOKI_LOG(1) << this->name() << " read " << result << " from position " << addr << endl;

  return result;
}

void Scratchpad::write(const MemoryAddr addr, const int32_t value) {
  Instrumentation::Scratchpad::write();
  data.write(value, addr);

  LOKI_LOG(1) << this->name() << " wrote " << value << " to position " << addr << endl;
}
