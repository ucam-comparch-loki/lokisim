/*
 * RoundRobinArbiter.h
 *
 *  Created on: 2 Mar 2011
 *      Author: db434
 */

#ifndef ROUNDROBINARBITER_H_
#define ROUNDROBINARBITER_H_

#include "ArbiterBase.h"

class RoundRobinArbiter: public ArbiterBase {

public:

  RoundRobinArbiter(int numInputs, int numOutputs, bool wormhole);

  virtual void arbitrateLogic(RequestList& requests, GrantList& grants);

private:

  int lastAccepted;

};

#endif /* ROUNDROBINARBITER_H_ */
