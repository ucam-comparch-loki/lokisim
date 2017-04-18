/*
 * FlowControlIn.h
 *
 * The Component responsible for handling flow control at a Component's input.
 * Jobs include receiving data and sending credits when buffer space becomes
 * available.
 *
 * Also handles dynamic channel allocation. Components may request an
 * exclusive connection with this channel, and will receive a 1 (accept) or
 * 0 (reject) as a response over the credit network.
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#ifndef FLOWCONTROLIN_H_
#define FLOWCONTROLIN_H_

#include "../../LokiComponent.h"
#include "../NetworkTypes.h"

class Word;

class FlowControlIn: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  // Credits are sent at the positive edge of the clock.
  ClockInput    clock;

  // Data received over the network, to be sent to a component's input.
  DataInput     iData;

  // Data to be sent to the component's input.
  sc_out<Word>  oData;

  // Responses to requests from each input.
  CreditOutput  oCredit;

  // Signal that a credit can be sent, if necessary. The signal switches each
  // time data is consumed from the buffer.
  ReadyInput    iDataConsumed;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(FlowControlIn);
  FlowControlIn(sc_module_name name, const ComponentID& ID, const ChannelID& channelManaged);

//============================================================================//
// Methods
//============================================================================//

private:

  // The main loop used to handle data: receives data, waits until there is
  // space in the buffer (if necessary), and sends acknowledgements.
  void dataLoop();

  // Helper functions for dataLoop.
  void handlePortClaim(NetworkData request);
  void acceptPortClaim(ChannelID source);
  void rejectPortClaim(ChannelID source);

  void addCredit();

  // The main loop used to handle credits: wait until there are credits to send,
  // send them, and wait for acknowledgements.
  void creditLoop();

  // Helper functions for creditLoop.
  void prepareCredit();
  void prepareNack();
  void sendCredit();

//============================================================================//
// Local state
//============================================================================//

private:

  // State machine for sending credits.
  enum CreditState {IDLE, CREDIT_SEND, ACKNOWLEDGE};

  CreditState creditState;

  // Address of channel managed by this flow control unit.
  const ChannelID sinkChannel;

  // Address of port connected to our input port.
  ChannelID sourceChannel;

  // Address to send negative acknowledgement to.
  ChannelID nackChannel;

  // The credit to be sent.
  NetworkCredit toSend;

  bool useCredits;
  unsigned int numCredits;

  bool disconnectPending;

  sc_event newCredit;

  // There are a couple of events which mean we can't accept any new input until
  // they have been processed fully. Trigger this event when those events end.
  sc_event unblockInput;

};

#endif /* FLOWCONTROLIN_H_ */
