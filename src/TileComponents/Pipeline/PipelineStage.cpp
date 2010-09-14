/*
 * PipelineStage.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "PipelineStage.h"
//#include "../Cluster.h"

Cluster* PipelineStage::parent() {
  return ((Cluster*)(this->get_parent()));
}

PipelineStage::PipelineStage(sc_module_name name) : Component(name) {

  SC_THREAD(newCycle);

}

PipelineStage::~PipelineStage() {

}
