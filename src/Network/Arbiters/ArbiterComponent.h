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

class AddressedWord;
class ArbiterBase;

class ArbiterComponent: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>            clock;

  sc_in<AddressedWord>  *dataIn;
  sc_out<AddressedWord> *dataOut;

  // Signal whether the data at each input port has been used yet.
  sc_in<bool>           *newData;

  // Signal to sender that a particular data item has been consumed. Signal by
  // toggling the value, rather than using true/false. Is this bad?
  sc_out<bool>          *readData;

  sc_in<bool>           *readyIn;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(ArbiterComponent);
  ArbiterComponent(sc_module_name name, ComponentID ID, int inputs, int outputs);
  virtual ~ArbiterComponent();

//==============================//
// Methods
//==============================//

private:

  void arbitrate();
  void dataArrived();

//==============================//
// Local state
//==============================//

private:

  ArbiterBase* arbiter;
  int numInputs, numOutputs;

  // Optimisation: skip work if we know there is no data to arbitrate.
  bool haveData;

};

#endif /* ARBITERCOMPONENT_H_ */
