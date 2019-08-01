/*
 * PipelineRegister.cpp
 *
 *  Created on: 15 Mar 2012
 *      Author: db434
 */

#include "PipelineRegister.h"

#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/PipelineReg.h"

bool PipelineRegister::canWrite() const {
  return !valid;
}

void PipelineRegister::write(const DecodedInst& inst) {
  if (ENERGY_TRACE)
    Instrumentation::PipelineReg::activity(data, inst, position);

  loki_assert(!valid);
  data = inst;
  valid = true;
  writeEvent.notify();
}

bool PipelineRegister::canRead() const {
  return valid;
}

const DecodedInst& PipelineRegister::read() {
  loki_assert(valid);
  readEvent.notify();
  valid = false;
  return data;
}

const sc_event& PipelineRegister::canReadEvent() const {
  return writeEvent;
}

const sc_event& PipelineRegister::canWriteEvent() const {
  return readEvent;
}

bool PipelineRegister::discard() {
  readEvent.notify();

  if (!valid)
    return false;
  else {
    valid = false;
    return true;
  }
}

PipelineRegister::PipelineRegister(const sc_module_name& name,
                                   const PipelinePosition pos) :
  LokiComponent(name),
  position(pos) {

  valid = false;

}
