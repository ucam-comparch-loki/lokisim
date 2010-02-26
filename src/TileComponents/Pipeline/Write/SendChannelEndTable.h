/*
 * SendChannelEndTable.h
 *
 * Component responsible for sending data out onto the network.
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#ifndef SENDCHANNELENDTABLE_H_
#define SENDCHANNELENDTABLE_H_

#include "../../../Component.h"
#include "../../../Datatype/Word.h"
#include "../../../Datatype/AddressedWord.h"
#include "../../../Datatype/Array.h"
#include "../../../Memory/Buffer.h"

class SendChannelEndTable: public Component {

public:
/* Ports */
  sc_in<Word>             input;
  sc_in<short>            remoteChannel;
  sc_in<bool>            *flowControl;  // array
  sc_out<AddressedWord>  *output;       // array

/* Constructors and destructors */
  SC_HAS_PROCESS(SendChannelEndTable);
  SendChannelEndTable(sc_core::sc_module_name name);
  virtual ~SendChannelEndTable();

protected:
/* Methods */
  void          doOp();
  virtual void  canSend();
  virtual short chooseBuffer();

/* Local state */
  vector<Buffer<AddressedWord> >  buffers;

};

#endif /* SENDCHANNELENDTABLE_H_ */
