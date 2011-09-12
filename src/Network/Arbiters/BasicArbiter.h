/*
 * BasicArbiter.h
 *
 * A SystemC wrapper for an arbiter from the Arbitration directory.
 *
 * This is the base component: it should be subclassed to allow different
 * methods of flow control.
 *
 *  Created on: 6 Sep 2011
 *      Author: db434
 */

#ifndef BASICARBITER_H_
#define BASICARBITER_H_

#include "../../Component.h"

class ArbiterBase;

class BasicArbiter: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>   clock;

  sc_in<bool>  *requests;
  sc_out<bool> *grants;

  sc_out<int>  *select;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(BasicArbiter);
  BasicArbiter(const sc_module_name& name, ComponentID ID,
               int inputs, int outputs, bool wormhole);
  virtual ~BasicArbiter();

//==============================//
// Methods
//==============================//

public:

  int numInputs() const;
  int numOutputs() const;

protected:

  // Returns a reference to an event which will be triggered when it is
  // possible to send data to the given output.
  virtual const sc_event& canGrantNow(int output) = 0;

  virtual void deassertGrant(int input, int output);

private:

  void arbitrate(int output);

  bool haveRequest() const;
  void requestChanged(int input);

//==============================//
// Local state
//==============================//

protected:

  const int inputs, outputs;
  bool wormhole;

private:

  // The class implementing the behaviour of this component. Can be easily
  // changed, without changing this component.
  ArbiterBase* arbiter;

  // Store requests in a separate structure which is cheaper to access, and
  // which can be easily shared with the behavioural model.
  std::vector<bool> requestVec, grantVec;
  sc_event receivedRequest;

};

#endif /* BASICARBITER_H_ */
