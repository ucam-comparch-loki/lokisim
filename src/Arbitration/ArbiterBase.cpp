/*
 * ArbiterBase.cpp
 *
 *  Created on: 2 Mar 2011
 *      Author: db434
 */

#include "ArbiterBase.h"
#include "RoundRobinArbiter.h"
#include <assert.h>

ArbiterBase* ArbiterBase::makeArbiter(ArbiterType type,
                                      const RequestList* requestVec,
                                      const GrantList* grantVec) {
  switch(type) {
    case ROUND_ROBIN: return new RoundRobinArbiter(requestVec, grantVec);
    case MATRIX:      assert(false);
    case PRIORITY:    assert(false);
    default:          assert(false);
  }
}

ArbiterBase::ArbiterBase(const RequestList* requestVec,
                         const GrantList* grantVec) {
  requests = requestVec;
  grants   = grantVec;
}
