/*
 * FlowControlIn.h
 *
 * The Component responsible for handling flow control at a Component's input.
 * Jobs include receiving data and sending credits when buffer space becomes
 * available.
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#ifndef FLOWCONTROLIN_H_
#define FLOWCONTROLIN_H_

#include "../../Component.h"

class AddressedWord;
class Word;

class FlowControlIn: public Component {

//==============================//
// Ports
//==============================//

public:

  // Credits are sent at the positive edge of the clock.
  sc_in<bool>           clock;

  // Data received over the network, to be sent to a component's input.
  sc_in<AddressedWord>  dataIn;
  sc_in<bool>           validDataIn;

  // Data to be sent to the component's input.
  sc_out<Word>          dataOut;

  // Responses to requests from each input.
  sc_out<AddressedWord> creditsOut;
  sc_out<bool>          validCreditOut;
  // keep this for the moment - credits still use old network system
  sc_in<bool>           ackCreditOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FlowControlIn);
  FlowControlIn(sc_module_name name, const ComponentID& ID, const ChannelID& channelManaged);

//==============================//
// Methods
//==============================//

private:

  // The main loop used to handle data: receives data, waits until there is
  // space in the buffer (if necessary), and sends acknowledgements.
  void dataLoop();

  // Helper functions for dataLoop.
  void handleNewData();
  void handlePortClaim();
  void addCredit();

  // The main loop used to handle credits: wait until there are credits to send,
  // send them, and wait for acknowledgements.
  void creditLoop();

  // Helper function for creditLoop.
  void sendCredit();

//==============================//
// Local state
//==============================//

private:

  // State machine for sending credits.
  enum CreditState {NO_CREDITS, WAITING_TO_SEND, WAITING_FOR_ACK};

  CreditState creditState;

  // Address of channel managed by this flow control unit.
  const ChannelID channel;

  // Address of port connected to each of our input port. We need the
  // address so we can send flow control information back to the source.
  ChannelID returnAddress;

  bool useCredits;
  unsigned int numCredits;

  sc_core::sc_event newCredit;

};

#endif /* FLOWCONTROLIN_H_ */
