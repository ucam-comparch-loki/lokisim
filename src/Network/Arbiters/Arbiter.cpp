/*
 * Arbiter.cpp
 *
 *  Created on: 16 Nov 2010
 *      Author: db434
 */

#include "Arbiter.h"
#include "NullArbiter.h"
#include "RoundRobinArbiter.h"

Arbiter* Arbiter::localDataArbiter(int numInputs, int numOutputs) {
  return new RoundRobinArbiter(numInputs, numOutputs);
}

Arbiter* Arbiter::localCreditArbiter(int numInputs, int numOutputs) {
  return new RoundRobinArbiter(numInputs, numOutputs);
}

Arbiter* Arbiter::globalDataArbiter(int numInputs, int numOutputs) {
  return new RoundRobinArbiter(numInputs, numOutputs);
}

Arbiter* Arbiter::globalCreditArbiter(int numInputs, int numOutputs) {
  return new RoundRobinArbiter(numInputs, numOutputs);
}

Arbiter::Arbiter(int numInputs, int numOutputs) {
  inputs = numInputs;
  outputs = numOutputs;
}

Arbiter::~Arbiter() {

}
