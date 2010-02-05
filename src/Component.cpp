/*
 * Component.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "Component.h"

Component::Component(sc_core::sc_module_name name) : sc_module(name) {

}

Component::Component(sc_core::sc_module_name name, int ID)
    : sc_module(name), id(ID) {

}

Component::Component(const Component& other) : sc_module("component") {
  id = other.id;
}

Component::~Component() {

}
