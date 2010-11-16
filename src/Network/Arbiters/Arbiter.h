/*
 * Arbiter.h
 *
 * The base class for all arbiters. Its job is to take a list of paths (each
 * specifying a source and a destination) and return a subset of them which can
 * safely complete simultaneously.
 *
 *  Created on: 16 Nov 2010
 *      Author: db434
 */

#ifndef ARBITER_H_
#define ARBITER_H_

#include <vector>
#include "Path.h"

using std::vector;

class Arbiter {

public:

  // Create arbiters of the appropriate types for particular networks.
  static Arbiter* localDataArbiter(int numInputs, int numOutputs);
  static Arbiter* localCreditArbiter(int numInputs, int numOutputs);
  static Arbiter* globalDataArbiter(int numInputs, int numOutputs);
  static Arbiter* globalCreditArbiter(int numInputs, int numOutputs);

  // Determine which inputs are allowed to send their data.
  virtual vector<Path>& arbitrate(vector<Path>& paths) = 0;

public:

  Arbiter(int numInputs, int numOutputs);
  virtual ~Arbiter();

protected:

  int inputs, outputs;

};

#endif /* ARBITER_H_ */
