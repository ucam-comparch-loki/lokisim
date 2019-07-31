/*
 * Scratchpad.cpp
 *
 *  Created on: 17 Jan 2013
 *      Author: db434
 */

#include "Scratchpad.h"
#include "../../../Utility/Instrumentation/Scratchpad.h"

#include "../../../Utility/Assert.h"
Scratchpad::Scratchpad(const sc_module_name& name,
                       const scratchpad_parameters_t& params) :
    LokiComponent(name),
    data(params.size) {
  // Do nothing.
}

int32_t Scratchpad::read(RegisterIndex addr) const {
  loki_assert_with_message(addr < data.size(), "Accessing position %d of %d", addr, data.size());

  Instrumentation::Scratchpad::read();
  int32_t result = data[addr];

  LOKI_LOG(1) << this->name() << " read " << result << " from position " << addr << endl;

  return result;
}

void Scratchpad::write(RegisterIndex addr, int32_t value) {
  loki_assert_with_message(addr < data.size(), "Accessing position %d of %d", addr, data.size());

  Instrumentation::Scratchpad::write();
  data[addr] = value;

  LOKI_LOG(1) << this->name() << " wrote " << value << " to position " << addr << endl;
}
