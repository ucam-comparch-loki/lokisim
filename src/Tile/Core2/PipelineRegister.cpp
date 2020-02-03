/*
 * PipelineRegister.cpp
 *
 *  Created on: 15 Mar 2012
 *      Author: db434
 */

#include "PipelineRegister.h"

#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/PipelineReg.h"

namespace Compute {

bool PipelineRegister::canWrite() const {
  return !(bool)data;
}

void PipelineRegister::write(const DecodedInstruction inst) {
//  if (ENERGY_TRACE)
//    Instrumentation::PipelineReg::activity(data, inst, position);

  loki_assert(canWrite());
  data = inst;
  writeEvent.notify(sc_core::SC_ZERO_TIME); // TODO: verify time delay
}

bool PipelineRegister::canRead() const {
  return (bool)data;
}

const DecodedInstruction PipelineRegister::read() {
  loki_assert(canRead());
  readEvent.notify(sc_core::SC_ZERO_TIME); // TODO: verify time delay
  const DecodedInstruction result = data;
  data.reset();
  return result;
}

const sc_event& PipelineRegister::canReadEvent() const {
  return writeEvent;
}

const sc_event& PipelineRegister::canWriteEvent() const {
  return readEvent;
}

bool PipelineRegister::discard() {
  readEvent.notify(sc_core::SC_ZERO_TIME); // TODO: verify time delay

  if (!canRead())
    return false;
  else {
    data.reset();
    return true;
  }
}

PipelineRegister::PipelineRegister(const sc_module_name& name,
                                   const PipelinePosition pos) :
    LokiComponent(name),
    position(pos) {

  // Nothing

}

} // end namespace
