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
  ArbiterComponent(sc_module_name name, const ComponentID& ID, int inputs, int outputs, bool wormhole);
  virtual ~ArbiterComponent();

//==============================//
// Methods
//==============================//

private:

  void mainLoop();

  void updateDataAvailable();

//==============================//
// Local state
//==============================//

private:

  int numInputs, numOutputs;

  int inactiveCycles;
  int activeTransfers;
  int lastAccepted; // Using round-robin at the moment.

  // Additional state for wormhole routing.

  bool wormhole;

  static const int NO_RESERVATION = -1;

  // For each input port, record which output to send the rest of the packet to.
  int* reservations;

  // Record whether each output port is available to send data on (it may be
  // reserved by another input).
  bool* reserved;

  // Record which outputs we are waiting for acknowledgements on.
  bool* inUse;

};

#endif /* ARBITERCOMPONENT_H_ */
