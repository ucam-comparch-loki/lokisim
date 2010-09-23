/*
 * PipelineStage.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "PipelineStage.h"

Cluster* PipelineStage::parent() const {
  return ((Cluster*)(this->get_parent()));
}

void PipelineStage::execute() {
  initialise();

  while(true) {
    newCycle();
    wait(clock.posedge_event());
  }
}

void PipelineStage::initialise() {
  // Default is to do nothing.
}

PipelineStage::PipelineStage(sc_module_name name) : Component(name) {

  SC_THREAD(execute);

}

PipelineStage::~PipelineStage() {

}
