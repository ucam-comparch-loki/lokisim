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
  // Initialisation usually involves tasks such as sending initial flow control
  // values.
  initialise();

  // Allow any signals to propagate before starting execution.
  wait(sc_core::SC_ZERO_TIME);

  // We then loop forever, executing the appropriate tasks each clock cycle.
  while(true) {
    newCycle();
    wait(clock.negedge_event());
    cycleSecondHalf();
    wait(clock.posedge_event());
  }
}

void PipelineStage::initialise() {
  // Default is to do nothing.
}

void PipelineStage::newCycle() {
  // Default is to do nothing.
}

void PipelineStage::cycleSecondHalf() {
  // Default is to do nothing.
}

PipelineStage::PipelineStage(sc_module_name name) : Component(name) {

  SC_THREAD(execute);

}

PipelineStage::~PipelineStage() {

}
