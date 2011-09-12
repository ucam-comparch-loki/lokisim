/*
 * ArbiterBase.h
 *
 * The base class for all other arbiters. Arbitrates over an any-to-any network.
 *
 *  Created on: 2 Mar 2011
 *      Author: db434
 */

#ifndef ARBITERBASE_H_
#define ARBITERBASE_H_

#include <vector>

typedef std::vector<bool> RequestList;
typedef std::vector<bool> GrantList;

class ArbiterBase {

public:

  static const int NO_GRANT = -1;

  // Returns the index of the request which should be granted next, or NO_GRANT
  // if none should be granted.
  virtual int getGrant() = 0;

  enum ArbiterType {ROUND_ROBIN, MATRIX, PRIORITY};

  static ArbiterBase* makeArbiter(ArbiterType type,
                                  const RequestList* requestVec,
                                  const GrantList* grantVec);

protected:

  ArbiterBase(const RequestList* requestVec, const GrantList* grantVec);

  int numInputs() const;

  // A pointer to the requests the arbiter has received.
  const RequestList* requests;

  // A pointer to the grants the arbiter has sent.
  const GrantList*   grants;

};

#endif /* ARBITERBASE_H_ */
