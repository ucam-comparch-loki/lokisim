/*
 * StageWithSuccessor.cpp
 *
 *  Created on: 27 Oct 2010
 *      Author: db434
 */

#include "StageWithSuccessor.h"
#include "../../Datatype/DecodedInst.h" // Not sure why this is needed

StageWithSuccessor::StageWithSuccessor(sc_module_name name, ComponentID ID) :
    PipelineStage(name, ID) {

}

StageWithSuccessor::~StageWithSuccessor() {

}
