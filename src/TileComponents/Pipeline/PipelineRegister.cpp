/*
 * PipelineRegister.cpp
 *
 *  Created on: 15 Mar 2012
 *      Author: db434
 */

#include "PipelineRegister.h"
#include "../../Utility/Instrumentation/PipelineReg.h"

bool PipelineRegister::ready() const {
//  return !buffer.full();
  return !valid;
}

void PipelineRegister::write(const DecodedInst& inst) {
  if (ENERGY_TRACE)
    Instrumentation::PipelineReg::activity(data, inst, position);

//  buffer.write(inst);
  assert(!valid);
  data = inst;
  valid = true;
  writeEvent.notify();
}

bool PipelineRegister::hasData() const {
//  return !buffer.empty();
  return valid;
}

const DecodedInst& PipelineRegister::read() {
//  return buffer.read();
  assert(valid);
  readEvent.notify();
  valid = false;
  return data;
}

const sc_event& PipelineRegister::dataAdded() const {
//  return buffer.writeEvent();
  return writeEvent;
}

const sc_event& PipelineRegister::dataRemoved() const {
//  return buffer.readEvent();
  return readEvent;
}

bool PipelineRegister::discard() {
  readEvent.notify();

//  if(buffer.empty())
  if (!valid)
    return false;
  else {
//    buffer.discardTop();
    valid = false;
    return true;
  }
}

PipelineRegister::PipelineRegister(const sc_module_name& name,
                                   const ComponentID ID,
                                   const PipelinePosition pos) :
  Component(name, ID),
//  buffer(1, std::string(name)) {  // Buffer has 2 spaces and a name for debug
  position(pos) {

}
