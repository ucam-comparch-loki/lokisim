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
  sc_out<bool>          ackDataIn;

  // Data to be sent to the component's input.
  sc_out<Word>          dataOut;

  // Responses to requests from each input.
  sc_out<AddressedWord> creditsOut;
  sc_out<bool>          validCreditOut;
  sc_in<bool>           ackCreditOut;

  // Flow control signal from the component's input, saying whether there is
  // room in the buffer for more data.
  sc_in<bool>           bufferHasSpace;

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

  // Data has arrived over the network.
  void receivedData();

  // Update the bufferHasSpace signal.
  void updateReady();

//==============================//
// Local state
//==============================//

private:

  enum ExecStateData {
	  STATE_DATA_INIT,
	  STATE_DATA_STANDBY,
	  STATE_DATA_BUFFER_FULL,
	  STATE_DATA_ACKNOWLEDGED
  };

  enum ExecStateCredits {
	  STATE_CREDITS_INIT,
	  STATE_CREDITS_STANDBY,
	  STATE_CREDITS_SENT
  };

  // Address of channel managed by this flow control unit.
  ChannelID channel;

  // Address of port connected to each of our input port. We need the
  // address so we can send flow control information back to the source.
  ChannelID returnAddress;

  bool useCredits;
  unsigned int numCredits;

  ExecStateData execStateData;
  unsigned long long execTimeData;

  ExecStateCredits execStateCredits;

  Word pendingData;

  sc_signal<int> triggerSignal;
  sc_signal<bool> creditsAvailable;
};

#endif /* FLOWCONTROLIN_H_ */
