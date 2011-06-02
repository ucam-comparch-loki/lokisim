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

PipelineStage::PipelineStage(sc_module_name name, const ComponentID& ID) :
    Component(name, ID) {

  SC_THREAD(execute);
  SC_THREAD(updateReady);

  idle.initialize(true);

}
