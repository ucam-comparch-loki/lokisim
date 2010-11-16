/*
 * NullArbiter.h
 *
 * The most basic arbiter. Performs no arbitration and allows all data to be
 * sent. This can therefore only be used when it is known to be safe.
 *
 *  Created on: 16 Nov 2010
 *      Author: db434
 */

#ifndef NULLARBITER_H_
#define NULLARBITER_H_

#include "Arbiter.h"

class NullArbiter: public Arbiter {

public:

  virtual vector<Path>& arbitrate(vector<Path>& paths);

public:

  NullArbiter(int numInputs, int numOutputs);
  virtual ~NullArbiter();

};

#endif /* NULLARBITER_H_ */
