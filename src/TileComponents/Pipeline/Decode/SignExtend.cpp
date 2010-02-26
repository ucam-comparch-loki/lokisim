/*
 * SignExtend.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SignExtend.h"

void SignExtend::doOp() {
  // Will it ever be necessary to allow immediates of different lengths?
  Data d = input.read();
  output.write(d);
}

SignExtend::SignExtend(sc_core::sc_module_name name) : Component(name) {

  SC_METHOD(doOp);
  sensitive << input;

}

SignExtend::~SignExtend() {

}
