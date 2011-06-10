/*
 * FlowControlOut.h
 *
 * The Component responsible for handling flow control at a Component's output.
 * Jobs include keeping track of credits, and sending data when we have more
 * than zero.
 *
 * NOTE: this code has been refactored, but is largely untested, as it is not
 * actively used in the current design.
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#ifndef FLOWCONTROLOUT_H_
#define FLOWCONTROLOUT_H_

#include "../../Component.h"

class AddressedWord;

class FlowControlOut: public Component {

//==============================//
// Ports
//==============================//

public:

  // Data received from the component to be sent out onto the network.
  sc_in<AddressedWord>   dataIn;

  // Flow control credits received over the network.
  sc_in<AddressedWord>   creditsIn;

  // Data sent out onto the network.
  sc_out<AddressedWord>  dataOut;

  // Signal from the network telling us that it is safe to send data.
  sc_in<bool>            readyIn;

  // Signal telling the network it is safe to send credits to this port.
  // Never actually used, but allows data and credit networks to have the same
  // interface.
  sc_out<bool>           readyOut;

  // A flow control signal for each output of the component, controlling when
  // new data is allowed to arrive.
  sc_out<bool>           flowControlOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FlowControlOut);
  FlowControlOut(sc_module_name name, const ComponentID& ID, const ChannelID& channelManaged);

//==============================//
// Methods
//==============================//

private:

  // Attempt to send new data whenever it arrives.
  void          mainLoop();

  // Write data to the network. This is known to be safe.
  void          sendData();

  void          handleNewData();

  // Update the credit count. Triggered whenever a new credit arrives.
  void          receivedCredit();

//==============================//
// Local state
//==============================//

private:

  enum FCOutState {WAITING_FOR_DATA, WAITING_FOR_CREDITS};

  FCOutState state;

  // Address of channel managed by this flow control unit.
  ChannelID channel;

  // Store the number of credits the output port has.
  unsigned int creditCount;

};

#endif /* FLOWCONTROLOUT_H_ */
