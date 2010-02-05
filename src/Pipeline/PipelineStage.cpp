/*
 * PipelineStage.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "PipelineStage.h"

PipelineStage::PipelineStage(sc_core::sc_module_name name, int ID)
    : Component(name, ID) {

  SC_METHOD(newCycle);
  sensitive << clock.pos();
  dont_initialize();

}

PipelineStage::~PipelineStage() {

}
