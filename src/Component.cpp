/*
 * Component.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include "Component.h"

Component::Component(const sc_module_name& name) : id(-1) {

}

Component::Component(const sc_module_name& name, ComponentID ID) :
    id(ID) {

}

double Component::area() const {
  std::cerr << "No area data for " << this->basename() << endl;
  return 0.0;
}

double Component::energy() const {
  std::cerr << "No power data for " << this->basename() << endl;
  return 0.0;
}
