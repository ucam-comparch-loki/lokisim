/*
 * InstructionReturn.cpp
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#include "InstructionReturn.h"

InstructionReturn::InstructionReturn(const sc_module_name name, ComponentID tile) :
Crossbar(name, tile, MEMS_PER_TILE, CORES_PER_TILE, 1, Network::COMPONENT, CORE_INSTRUCTION_CHANNELS) {
  // All initialisation handled by Crossbar constructor.

}

InstructionReturn::~InstructionReturn() {
  // Nothing
}

