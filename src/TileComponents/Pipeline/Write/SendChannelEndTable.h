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
#include "../../../Memory/BufferArray.h"

class SendChannelEndTable: public Component {

public:

/* Ports */

  // Clock.
  sc_in<bool>             clock;

  // The data (remote instruction or result from ALU) to be written into
  // this table.
  sc_in<Word>             input;

  // The remote channel the data is to be sent to.
  sc_in<short>            remoteChannel;

  // The outputs to the network. There should be NUM_SEND_CHANNELS of them.
  sc_out<AddressedWord>  *output;

  // A flow control signal for each output (NUM_SEND_CHANNELS), saying
  // whether or not it is allowed to send its next value.
  sc_in<bool>            *flowControl;

  // Signal to the cluster that it should stop execution until there is at
  // least one space in every buffer.
  sc_out<bool>            stallOut;

/* Constructors and destructors */
  SC_HAS_PROCESS(SendChannelEndTable);
  SendChannelEndTable(sc_module_name name);
  virtual ~SendChannelEndTable();

protected:
/* Methods */
  void          receivedData();
  void          updateStall();
  virtual void  canSend();
  virtual short chooseBuffer();

/* Local state */
  BufferArray<AddressedWord> buffers;
  bool stallValue;

/* Signals (wires) */
  sc_buffer<bool>         updateStall1, updateStall2;

};

#endif /* SENDCHANNELENDTABLE_H_ */
