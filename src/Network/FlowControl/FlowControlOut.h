/*
 * FlowControlOut.h
 *
 * The Component responsible for handling flow control at a Component's output.
 * Jobs include keeping track of credits, and sending data when we have more
 * than zero.
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
  sc_out<bool>           readyOut;

  // A flow control signal for each output of the component, controlling when
  // new data is allowed to arrive.
  sc_out<bool>           flowControlOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FlowControlOut);
  FlowControlOut(sc_module_name name, ComponentID ID);
  virtual ~FlowControlOut();

//==============================//
// Methods
//==============================//

protected:

  // Attempt to send new data whenever it arrives.
  void          mainLoop();

  // Attempt to send any newly-received data, blocking until credits arrive if
  // necessary.
  void          sendData();

  // Update the credit count. Triggered whenever a new credit arrives.
  void          receivedCredit();

//==============================//
// Local state
//==============================//

protected:

  // Store the number of credits the output port has.
  int  creditCount;

};

#endif /* FLOWCONTROLOUT_H_ */
