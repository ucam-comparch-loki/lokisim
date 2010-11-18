/*
 * LeastRecentlyServedArbiter.cpp
 *
 *  Created on: 18 Nov 2010
 *      Author: db434
 */

#include "LeastRecentlyServedArbiter.h"

LeastRecentlyServedArbiter::LeastRecentlyServedArbiter(int numInputs, int numOutputs) :
    Arbiter(numInputs, numOutputs) {
  // TODO: use queue to determine which inputs were served least recently.
}

LeastRecentlyServedArbiter::~LeastRecentlyServedArbiter() {

}
