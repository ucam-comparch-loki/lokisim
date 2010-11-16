/*
 * RoundRobinArbiter.h
 *
 *  Created on: 16 Nov 2010
 *      Author: db434
 */

#ifndef ROUNDROBINARBITER_H_
#define ROUNDROBINARBITER_H_

#include "Arbiter.h"

class RoundRobinArbiter: public Arbiter {

public:

  virtual vector<Path>& arbitrate(vector<Path>& paths);

public:

  RoundRobinArbiter(int numInputs, int numOutputs);
  virtual ~RoundRobinArbiter();

private:

  // The source which we most-recently allowed to send data. Keeping track of
  // this allows us to continue from the position required to implement a
  // round-robin scheme.
  int lastAccepted;

};

#endif /* ROUNDROBINARBITER_H_ */
