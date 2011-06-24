/*
 * PipelineStage.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "PipelineStage.h"
#include "../Cluster.h"

Cluster* PipelineStage::parent() const {
  return static_cast<Cluster*>(this->get_parent());
}

PipelineStage::PipelineStage(sc_module_name name, const ComponentID& ID) :
    Component(name, ID) {

  SC_METHOD(updateReady);

  idle.initialize(true);

}
