/*
 * ArbiterComponent.h
 *
 * It is assumed that all inputs will arrive in the same delta-cycle. This is
 * usually true, since the inputs will come from the same component, but it is
 * worth bearing in mind.
 *
 * Arbiters send data on the negative clock edge to allow time for any data to
 * arrive. Data is always sent from source components on the positive clock edge.
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
  ArbiterComponent(const sc_module_name& name, const ComponentID& ID, int inputs, int outputs, bool wormhole);
  virtual ~ArbiterComponent();

//==============================//
// Methods
//==============================//

public:

  unsigned int numInputs() const;
  unsigned int numOutputs() const;

private:

  // Main loop responsible for sending/receiving data and dealing with
  // acknowledgements.
  void arbiterLoop();

  // Helper functions for arbiterLoop. arbitrate() currently implements round-
  // robin scheduling.
  void arbitrate();
  bool haveData();

  void receivedAck();
  void sendAck(PortIndex input);

  void newData(PortIndex input);

//==============================//
// Local state
//==============================//

private:

  enum ArbiterState {WAITING_FOR_DATA, WAITING_FOR_ACKS};

  ArbiterState state;

  const unsigned int inputs, outputs;

  unsigned int activeTransfers;
  unsigned int numValidInputs;
  unsigned int lastAccepted; // Using round-robin at the moment.

  // Record which outputs we are waiting for acknowledgements on.
  bool* inUse;

  // Record which inputs we have accepted data from. We only acknowledge data
  // when the output has acknowledged it, so there may be a period of time
  // where the "valid" signal is high, but we have already used the data.
  bool* alreadySeen;

  // Record where to send acknowledgements to once the output data has been
  // consumed. (Would this storage be needed in practice?)
  int*  ackDestinations;

  // Event which is triggered whenever new data arrives. It uses the validDataIn
  // signals, so it doesn't notice the new data if the valid signal doesn't
  // change.
  sc_core::sc_event newDataEvent;

  // Events to notify to send an acknowledgement on each input.
  sc_core::sc_event* sendAckEvent;

  // Additional state for wormhole routing.

  const bool wormhole;

  static const unsigned int NO_RESERVATION = -1;

  // For each input port, record which output to send the rest of the packet to.
  unsigned int* reservations;

  // Record whether each output port is available to send data on (it may be
  // reserved by another input).
  bool* reserved;

};

#endif /* ARBITERCOMPONENT_H_ */
