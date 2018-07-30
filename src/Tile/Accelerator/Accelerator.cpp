/*
 * Accelerator.cpp
 *
 *  Created on: 2 Jul 2018
 *      Author: db434
 */

#include "Accelerator.h"

Accelerator::Accelerator(sc_module_name name, ComponentID id) :
    LokiComponent(name, id),
    ControlUnit(name),
    in1(name),
    in2(name),
    out(name) {

  // TODO Create components and wire them up.

}
