/*
 * PipelineRegister.cpp
 *
 *  Created on: 15 Mar 2012
 *      Author: db434
 */

#include "PipelineRegister.h"

bool PipelineRegister::ready() const {
  return !buffer.full();
}

void PipelineRegister::write(const DecodedInst& inst) {
  buffer.write(inst);
}

bool PipelineRegister::hasData() const {
  return !buffer.empty();
}

const DecodedInst& PipelineRegister::read() {
  return buffer.read();
}

const sc_event& PipelineRegister::dataArrived() const {
  return buffer.writeEvent();
}

const sc_event& PipelineRegister::dataLeft() const {
  return buffer.readEvent();
}

bool PipelineRegister::discard() {
  if(buffer.empty())
    return false;
  else {
    buffer.discardTop();
    return true;
  }
}

PipelineRegister::PipelineRegister(const sc_module_name& name, const ComponentID ID) :
  Component(name, ID),
  buffer(2, std::string(name)) {  // Buffer has 2 spaces and a name for debug

}
