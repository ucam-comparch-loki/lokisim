/*
 * PredicateRegister.cpp
 *
 *  Created on: 22 Mar 2010
 *      Author: db434
 */

#include "PredicateRegister.h"

bool PredicateRegister::getVal() const {
  return predicate;
}

void PredicateRegister::writeVal() {
  predicate = write.read();
  output.write(predicate);
}

PredicateRegister::PredicateRegister(sc_module_name name) : Component(name) {

  predicate = false;

  SC_METHOD(writeVal);
  sensitive << write;
  dont_initialize();

}

PredicateRegister::~PredicateRegister() {

}
