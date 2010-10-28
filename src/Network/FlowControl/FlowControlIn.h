/*
 * FlowControlIn.h
 *
 * The Component responsible for handling flow control at a Component's input.
 * Jobs include receiving Requests and sending responses to say if the transfer
 * is possible.
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#ifndef FLOWCONTROLIN_H_
#define FLOWCONTROLIN_H_

#include "../../Component.h"
#include "../../Datatype/AddressedWord.h"

class Request;
class Word;

class FlowControlIn: public Component {

//==============================//
// Ports
//==============================//

public:

  // Data received over the network, to be sent to the component's inputs.
  // The array should be "width" elements long.
  sc_in<AddressedWord>  *dataIn;

  // Responses to requests from each input ("width" elements long).
  sc_out<AddressedWord> *credits;

  // Flow control signals from each of the components inputs, saying how
  // much space is in their buffers (array is "width" elements long).
  sc_in<int>            *flowControl;

  // Data to be sent to each of the component's inputs ("width" elements).
  sc_out<Word>          *dataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FlowControlIn);
  FlowControlIn(sc_module_name name, int width);
  virtual ~FlowControlIn();

//==============================//
// Methods
//==============================//

protected:

  // Data has arrived over the network.
  void         receivedData();

  // The component has updated its flow control signals.
  void         receivedFlowControl();

//==============================//
// Local state
//==============================//

protected:

  // The number of inputs and outputs of this component.
  const int         width;

  // Shows which ports are free to accept new connection requests.
  std::vector<int>  flitsRemaining;

  // Addresses of ports connected to each of our input ports. We need the
  // addresses so we can send flow control information back to them.
  std::vector<ChannelID>  returnAddresses;

  // Rather than credits, the component sends out the amount of space in each
  // of its buffers. This information can be used to implement any other flow
  // control mechanism. In this case, we store the previous count, so that we
  // can detect when there is an extra space, and send a credit.
  std::vector<int>  bufferSpace;

};

#endif /* FLOWCONTROLIN_H_ */
