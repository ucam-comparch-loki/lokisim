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

  RoundRobinArbiter(const RequestList* requestVec, const GrantList* grantVec);

  virtual int getGrant();

private:

  int lastAccepted;

};

#endif /* ROUNDROBINARBITER_H_ */
