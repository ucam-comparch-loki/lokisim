/*
 * FetchStage.cpp
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#include "FetchStage.h"

namespace Compute {

FetchStage::FetchStage(sc_module_name name) :
    PipelineStage(name) {
  // TODO: get instruction from cache/fifo
  // Copy across most of old FetchStage?
}

void FetchStage::execute() {
  // Nothing to do
}

} // end namespace

