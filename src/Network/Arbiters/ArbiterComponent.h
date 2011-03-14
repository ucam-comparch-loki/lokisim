/*
 * ArbiterComponent.h
 *
 * An arbitrated multiplexer with input/output ports. Note that the arbiters
 * from Arbitration are used: the ones in this directory are to be phased out.
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

  sc_inout<bool>        *newData;
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
