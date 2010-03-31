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
#include "../../Datatype/Data.h"
#include "../../Datatype/Request.h"
#include "../../Memory/BufferArray.h"

class FlowControlIn: public Component {

//==============================//
// Ports
//==============================//

public:

  // Data received over the network, to be sent to the component's inputs.
  // The array should be "width" elements long.
  sc_in<Word>           *dataIn;

  // Requests to send data to each of the "width" inputs.
  sc_in<Word>           *requests;

  // Responses to requests from each input ("width" elements long).
  sc_out<AddressedWord> *responses;

  // Flow control signals from each of the components inputs, saying whether
  // they are ready to receive the next piece of data ("width" elements long).
  sc_in<bool>           *flowControl;

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

  virtual void receivedFlowControl();
  virtual void receivedRequests();
  void         receivedData();
  virtual bool acceptRequest(Request r, int input);
  void         setup();

//==============================//
// Local state
//==============================//

protected:

  // The network's output buffers.
  BufferArray<Word> buffers;

  // Signals that we have new data and want to send it.
  sc_buffer<bool>   tryToSend;

  // The number of inputs and outputs of this component.
  const int         width;

  // Shows which ports are free to accept new connection requests.
  std::vector<int>  flitsRemaining;

};

#endif /* FLOWCONTROLIN_H_ */
