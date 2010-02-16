/*
 * FlowControlIn.h
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#ifndef FLOWCONTROLIN_H_
#define FLOWCONTROLIN_H_

#include "../../Component.h"
#include "../../Datatype/Array.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/Data.h"
#include "../../Datatype/Request.h"
#include "../../Memory/Buffer.h"

class FlowControlIn: public Component {

public:
/* Ports */
  sc_in<AddressedWord>  *dataIn, *requests;
  sc_in<bool>           *flowControl;
  sc_out<AddressedWord> *dataOut, *responses;

/* Constructors and destructors */
  SC_HAS_PROCESS(FlowControlIn);
  FlowControlIn(sc_core::sc_module_name name, int ID, int width);
  virtual ~FlowControlIn();

protected:
/* Methods */
  virtual void receivedFlowControl();
  virtual void receivedRequests();
  void         receivedData();

/* Local state */
  std::vector<Buffer<AddressedWord> > buffers;
  int width;

};

#endif /* FLOWCONTROLIN_H_ */
