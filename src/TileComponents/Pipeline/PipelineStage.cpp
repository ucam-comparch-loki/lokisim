/*
 * PipelineStage.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "PipelineStage.h"

PipelineStage::PipelineStage(sc_core::sc_module_name name)
    : Component(name) {

  SC_THREAD(newCycle);

}

PipelineStage::~PipelineStage() {

}
