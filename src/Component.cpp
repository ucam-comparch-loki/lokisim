/*
 * Component.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "Component.h"

Component::Component(sc_module_name& name) : sc_module(name) {

}


/* NOTE: if this constructor is used, end_module() must be placed at
 *       the end of the constructor this is called from. */
Component::Component(sc_module_name& name, int ID)
    : sc_module(makeName(name, ID)), id(ID) {

}

Component::~Component() {

}

std::string Component::makeName(sc_module_name& name, int ID) {
  std::stringstream ss;
  std::string result;

  ss << name << "_" << ID;
  ss >> result;

  return result;
}
