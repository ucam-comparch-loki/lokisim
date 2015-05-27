/*
 * Component.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "Component.h"

Component::Component(const sc_module_name& name) : id(0,0,0) {

}

Component::Component(const sc_module_name& name, const ComponentID& ID) :
    id(ID) {

}
