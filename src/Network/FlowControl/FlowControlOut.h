/*
 * FlowControlOut.h
 *
 * The Component responsible for handling flow control at a Component's output.
 * Jobs include sending Requests and then sending data if the response comes
 * back positive.
 *
 *  Created on: 15 Feb 2010
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

  // Clock.
  sc_in<bool>             clock;

  // Data received from the component to be sent out onto the network. There
  // should be "width" elements of the array.
  sc_in<AddressedWord>   *dataIn;

  // Requests sent out onto the network to see if it is possible to send the
  // data yet ("width" elements).
  sc_out<AddressedWord>  *requests;

  // Responses to the requests ("width" elements).
  sc_in<AddressedWord>   *responses;

  // Data sent out onto the network after a positive response ("width" elements).
  sc_out<AddressedWord>  *dataOut;

  // A flow control signal for each output ("width" elements).
  sc_out<bool>           *flowControl;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FlowControlOut);
  FlowControlOut(sc_module_name name, int ID, int width);
  virtual ~FlowControlOut();

//==============================//
// Methods
//==============================//

public:

  void          initialise();

protected:

  void          sendData();
  void          receivedFlowControl();

  void          receivedResponses();
  virtual void  allowedToSend(int position, bool isAllowed);
  virtual void  sendRequests();

//==============================//
// Local state
//==============================//

protected:

  const int         width;

  // A bit to show whether we are waiting to request to send data from each
  // input. The usual event() approach cannot be used, as the data may be quite
  // old, having had its requests turned town.
  std::vector<bool> waitingToSend;

  // Store the number of credits each output port has.
  std::vector<int>  credits;

};

#endif /* FLOWCONTROLOUT_H_ */
