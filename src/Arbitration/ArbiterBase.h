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

#include <utility>
#include <vector>

// int = input, bool = is final flit in packet
typedef std::pair<int, bool> Request;

// int = input, int = output
typedef std::pair<int, int> Grant;

typedef std::vector<Request> RequestList;
typedef std::vector<Grant> GrantList;

class ArbiterBase {

public:

  void arbitrate(RequestList& requests, GrantList& grants);

protected:

  ArbiterBase(int numInputs, int numOutputs, bool useWormhole);

  virtual void arbitrateLogic(RequestList& requests, GrantList& grants) = 0;

  // Look through the reservations to see if this request can send, and add
  // a grant to the list if necessary.
  void wormholeGrant(Request& request, GrantList& grants);

  // Allow a given request to send its data to the given output.
  void grant(Request& request, int output, GrantList& grants);

  unsigned int inputs, outputs;
  bool wormhole;

private:

  // Record which inputs have reserved each output. This is used to implement
  // wormhole routing, where once a packet starts being sent, it should not
  // be interrupted.
  std::vector<int> reservations;

  static const int NO_RESERVATION = -1;

  // Record which outputs have been used this cycle so we don't use them
  // multiple times.
  std::vector<bool> outputsUsed;

};

#endif /* ARBITERBASE_H_ */
