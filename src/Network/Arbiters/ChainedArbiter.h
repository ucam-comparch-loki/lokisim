/*
 * ChainedArbiter.h
 *
 * An arbiter which is not the last in a chain of arbiters, and so must itself
 * issue a request and receive a grant from the next arbiter.
 *
 * This is useful in the network where all of the subnetworks converge at each
 * core: each subnetwork can have its own arbiter, and each core can have
 * another arbiter to choose between the networks.
 *
 *  Created on: 6 Sep 2011
 *      Author: db434
 */

#ifndef CHAINEDARBITER_H_
#define CHAINEDARBITER_H_

#include "BasicArbiter.h"

class ChainedArbiter: public BasicArbiter {

//==============================//
// Ports
//==============================//

public:

// Inherited from BasicArbiter:
//  sc_in<bool>   clock;
//  RequestInput *requests;
//  GrantOutput  *grants;
//  SelectOutput *select;

  // Connections to the next arbiter in the chain.
  RequestOutput *requestOut;
  GrantInput    *grantIn;

//==============================//
// Constructors and destructors
//==============================//

public:

  ChainedArbiter(const sc_module_name& name, ComponentID ID,
                 int inputs, int outputs, bool wormhole);
  virtual ~ChainedArbiter();

//==============================//
// Methods
//==============================//

protected:

  virtual const sc_event& canGrantNow(int output, const ChannelIndex destination);
  virtual const sc_event& stallGrant(int output);

  virtual void deassertGrant(int input, int output);

};

#endif /* CHAINEDARBITER_H_ */
