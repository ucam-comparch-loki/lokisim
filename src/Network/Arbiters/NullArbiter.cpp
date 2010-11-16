/*
 * NullArbiter.cpp
 *
 *  Created on: 16 Nov 2010
 *      Author: db434
 */

#include "NullArbiter.h"

vector<Path>& NullArbiter::arbitrate(vector<Path>& paths) {
  return paths;
}

NullArbiter::NullArbiter(int numInputs, int numOutputs) :
    Arbiter(numInputs, numOutputs) {

}

NullArbiter::~NullArbiter() {

}
