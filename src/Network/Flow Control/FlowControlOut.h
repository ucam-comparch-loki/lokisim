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
#include "../../Datatype/Array.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/Request.h"

class FlowControlOut: public Component {

public:
/* Ports */
  sc_in<bool>             clock;
  sc_in<AddressedWord>   *dataIn;                // array
  sc_in<bool>            *responses;             // array
  sc_out<AddressedWord>  *dataOut, *requests;    // array
  sc_out<bool>           *flowControl;           // array

/* Constructors and destructors */
  SC_HAS_PROCESS(FlowControlOut);
  FlowControlOut(sc_core::sc_module_name name, int ID, int width);
  virtual ~FlowControlOut();

protected:
/* Methods */
  void          newCycle();
  virtual void  allowedToSend(int position, bool isAllowed);
  virtual void  sendRequests();

/* Local state */
  int width;

};

#endif /* FLOWCONTROLOUT_H_ */
