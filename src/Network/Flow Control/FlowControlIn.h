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

public:
/* Ports */
  sc_in<Word>           *dataIn, *requests;
  sc_in<bool>           *flowControl;
  sc_out<Word>          *dataOut;
  sc_out<AddressedWord> *responses;

/* Constructors and destructors */
  SC_HAS_PROCESS(FlowControlIn);
  FlowControlIn(sc_module_name name, int width);
  virtual ~FlowControlIn();

protected:
/* Methods */
  virtual void receivedFlowControl();
  virtual void receivedRequests();
  void         receivedData();
  virtual bool acceptRequest(Request r, int input);
  void         setup();

/* Local state */
  BufferArray<Word> buffers;   // The network's output buffers
  sc_buffer<bool>   tryToSend; // Signals that we have new data and want to send it
  int               width;     // The number of inputs and outputs

};

#endif /* FLOWCONTROLIN_H_ */
