/*
 * Component.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "LokiComponent.h"

LokiComponent::LokiComponent(const sc_module_name& name) : id(0,0,0) {

}

LokiComponent::LokiComponent(const sc_module_name& name, const ComponentID& ID) :
    id(ID) {

}
