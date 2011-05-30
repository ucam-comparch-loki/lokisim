/*
 * ArbiterComponent.h
 *
 * An arbitrated multiplexer with input/output ports. Note that the arbiters
 * from Arbitration are used: the ones in this directory are to be phased out.
 *
 * It is assumed that all inputs will arrive in the same delta-cycle. This is
 * usually true, since the inputs will come from the same component, but it is
 * worth bearing in mind.
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#ifndef ARBITERCOMPONENT_H_
#define ARBITERCOMPONENT_H_

#include "../../Component.h"

#include <utility>
#include <vector>

// int = input, bool = is final flit in packet
typedef std::pair<int, bool> Request;

// int = input, int = output
typedef std::pair<int, int> Grant;

typedef std::vector<Request> RequestList;
typedef std::vector<Grant> GrantList;

class AddressedWord;

class ArbiterComponent: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>            clock;

  sc_in<AddressedWord>  *dataIn;
  sc_in<bool>           *validDataIn;
  sc_out<bool>          *ackDataIn;

  sc_out<AddressedWord> *dataOut;
  sc_out<bool>          *validDataOut;
  sc_in<bool>           *ackDataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(ArbiterComponent);
  ArbiterComponent(sc_module_name name, const ComponentID& ID, int inputs, int outputs);
  virtual ~ArbiterComponent();

//==============================//
// Methods
//==============================//

private:

  void arbitrateProcess();

  void arbitrate();

//==============================//
// Local state
//==============================//

private:

  enum ArbiterState {
	  STATE_INIT,
	  STATE_STANDBY,
	  STATE_TRIGGERED,
	  STATE_TRANSFER
  };

  int numInputs, numOutputs;

  sc_signal<ArbiterState> currentState;
  sc_signal<int> activeTransfers;
  sc_signal<int> lastAccepted;
};

#endif /* ARBITERCOMPONENT_H_ */
