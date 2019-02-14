/*
 * InstructionReturn.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "InstructionReturn.h"
#include "../Core/Core.h"

InstructionReturn::InstructionReturn(const sc_module_name name,
                                     const tile_parameters_t& params) :
Crossbar(name, params.numMemories, params.numCores, 1, Core::numInstructionChannels) {
  // All initialisation handled by Crossbar constructor.

}

InstructionReturn::~InstructionReturn() {
  // Nothing
}

