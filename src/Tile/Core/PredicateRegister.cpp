/*
 * PredicateRegister.cpp
 *
 *  Created on: 22 Mar 2010
 *      Author: db434
 */

#include "../../Tile/Core/PredicateRegister.h"

bool PredicateRegister::read() const {
  return predicate;
}

void PredicateRegister::write(bool val) {
  predicate = val;
  LOKI_LOG(1) << this->name() << " set to " << val << endl;
}

PredicateRegister::PredicateRegister(const sc_module_name& name) : LokiComponent(name) {

  predicate = false;

}
