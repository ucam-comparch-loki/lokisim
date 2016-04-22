/*
 * ClockedArbiter.h
 *
 * A SystemC wrapper for an arbiter from the Arbitration directory.
 *
 * Arbiters perform arbitration on the negative clock edge, so all requests
 * must be received by then.
 * The select signals, telling the multiplexers which inputs to use, are
 * written on the positive edge.
 *
 *  Created on: 6 Sep 2011
 *      Author: db434
 */

#ifndef BASICARBITER_H_
#define BASICARBITER_H_

#include "../../Component.h"
#include "../../Utility/BlockingInterface.h"
#include "../NetworkTypedefs.h"

class ArbiterBase;

class ClockedArbiter: public Component, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput   clock;

  LokiVector<ArbiterRequestInput> iRequest;
  LokiVector<ArbiterGrantOutput>  oGrant;

  LokiVector<MuxSelectOutput> oSelect;

  // Signals from the component telling if it is able to receive more data.
  LokiVector<ReadyInput> iReady;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(ClockedArbiter);
  ClockedArbiter(const sc_module_name& name, ComponentID ID,
               int inputs, int outputs, bool wormhole, int flowControlSignals);
  virtual ~ClockedArbiter();

//============================================================================//
// Methods
//============================================================================//

public:

  int numInputs() const;
  int numOutputs() const;

protected:

  // Returns a reference to an event which will be triggered when it is
  // possible to send data to the given output.
  virtual const sc_event& canGrantNow(int output, const ChannelIndex destination);

  // An event which is triggered if the grant must be temporarily removed,
  // because the destination is not capable of receiving more data yet.
  virtual const sc_event& stallGrant(int output);

  virtual void grant(int input, int output);
  virtual void deassertGrant(int input, int output);

  // The task to perform whenever a request input changes.
  virtual void requestChanged(int input);

  virtual void reportStalls(ostream& os);

private:

  // Return which ready signal dictates when the input data will be allowed
  // to send.
  PortIndex outputToUse(PortIndex input);

  // Change a particular select output to a particular value.
  void changeSelection(int output, MuxSelect value);

  void arbitrate(int output);

  bool haveRequest() const;

  void updateGrant(int input);
  void updateSelect(int output);

//============================================================================//
// Local state
//============================================================================//

protected:

  bool wormhole;

  // Store values of all inputs/outputs in vectors. This makes the information
  // cheaper to access, and makes it easier to share with the behavioural model.
  std::vector<bool> requestVec, grantVec;
  std::vector<MuxSelect> selectVec;
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
  LokiVector<sc_event> grantChanged, selectionChanged;

  // An event which is notified whenever canGrantNow determines that it is safe
  // to grant a request.
  sc_event grantEvent;

};

#endif /* BASICARBITER_H_ */
