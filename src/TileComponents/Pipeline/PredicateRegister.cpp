/*
 * PredicateRegister.cpp
 *
 *  Created on: 22 Mar 2010
 *      Author: db434
 */

#include "PredicateRegister.h"

bool PredicateRegister::read() const {
  return predicate;
}

void PredicateRegister::write(bool val) {
  predicate = val;
  if(DEBUG) cout << this->name() << " set to " << val << endl;
}

PredicateRegister::PredicateRegister(sc_module_name name) : Component(name) {

  predicate = false;

}

PredicateRegister::~PredicateRegister() {

}
