/*
 * Component.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "Component.h"

Component::Component(sc_module_name& name) : sc_module(name), id(-1) {

}


/* NOTE: if this constructor is used, end_module() must be placed at
 *       the end of the constructor this is called from. */
Component::Component(sc_module_name& name, int ID)
    : sc_module(makeName(name, ID)), id(ID) {

}

Component::~Component() {

}

double Component::area() const {
  std::cerr << "No area data for " << this->basename() << endl;
  return 0.0;
}

double Component::energy() const {
  std::cerr << "No power data for " << this->basename() << endl;
  return 0.0;
}

std::string Component::makeName(sc_module_name& name, int ID) {
  std::stringstream ss;
  std::string result;

  ss << name << "_" << ID;
  ss >> result;

  return result;
}
