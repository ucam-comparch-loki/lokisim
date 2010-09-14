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
}

PredicateRegister::PredicateRegister(sc_module_name name) : Component(name) {

  predicate = false;

}

PredicateRegister::~PredicateRegister() {

}
