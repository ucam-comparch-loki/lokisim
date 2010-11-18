/*
 * LeastRecentlyServedArbiter.h
 *
 *  Created on: 18 Nov 2010
 *      Author: db434
 */

#ifndef LEASTRECENTLYSERVEDARBITER_H_
#define LEASTRECENTLYSERVEDARBITER_H_

#include "Arbiter.h"

class LeastRecentlyServedArbiter: public Arbiter {
public:
  LeastRecentlyServedArbiter(int numInputs, int numOutputs);
  virtual ~LeastRecentlyServedArbiter();
};

#endif /* LEASTRECENTLYSERVEDARBITER_H_ */
