/*
 * BasicArbiter.h
 *
 * A SystemC wrapper for an arbiter from the Arbitration directory.
 *
 * Arbiters perform arbitration on the negative clock edge, so all requests
 * must be received by then.
 * The select signals, telling the multiplexers which inputs to use, are
 * written on the positive edge.
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
#include "../NetworkTypedefs.h"

class ArbiterBase;

class BasicArbiter: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>   clock;

  RequestInput *requests;
  GrantOutput  *grants;

  SelectOutput *select;

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
  virtual const sc_event& canGrantNow(int output, const ChannelIndex destination) = 0;

  // An event which is triggered if the grant must be temporarily removed,
  // because the destination is not capable of receiving more data yet.
  virtual const sc_event& stallGrant(int output) = 0;

  virtual void deassertGrant(int input, int output);

  // The task to perform whenever a request input changes.
  virtual void requestChanged(int input);

private:

  // Change a particular select output to a particular value.
  void changeSelection(int output, SelectType value);

  void arbitrate(int output);

  bool haveRequest() const;

  void updateGrant(int input);
  void updateSelect(int output);

//==============================//
// Local state
//==============================//

protected:

  const int inputs, outputs;
  bool wormhole;

  // Store values of all inputs/outputs in vectors. This makes the information
  // cheaper to access, and makes it easier to share with the behavioural model.
  std::vector<bool> requestVec, grantVec;
  std::vector<SelectType> selectVec;
  sc_event receivedRequest;

private:

  enum ArbiterState {
    NO_REQUESTS,
    HAVE_REQUESTS,
    WAITING_TO_GRANT,
    GRANTED
  };

  // We have a separate process for each output port, so store the state of
  // each one.
  std::vector<ArbiterState> state;

  // The class implementing the behaviour of this component. Can be easily
  // changed, without changing this component.
  ArbiterBase* arbiter;

  // Array of events which tell when to change each grant signal.
  sc_event *grantChanged;
  sc_event *selectionChanged;

};

#endif /* BASICARBITER_H_ */
