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

//==============================//
// Ports
//==============================//

public:

  // Clock.
  sc_in<bool>             clock;

  // The data (remote instruction or result from ALU) to be written into
  // this table.
  sc_in<Word>             input;

  // The remote channel the data is to be sent to.
  sc_in<short>            remoteChannel;

  // Stall the pipeline until this channel has become empty. This allows
  // synchronisation between clusters.
  sc_in<short>            waitOnChannel;

  // The outputs to the network. There should be NUM_SEND_CHANNELS of them.
  sc_out<AddressedWord>  *output;

  // A flow control signal for each output (NUM_SEND_CHANNELS), saying
  // whether or not it is allowed to send its next value.
  sc_in<bool>            *flowControl;

  // Signal to the cluster that it should stop execution until there is at
  // least one space in every buffer.
  sc_out<bool>            stallOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(SendChannelEndTable);
  SendChannelEndTable(sc_module_name name);
  virtual ~SendChannelEndTable();

//==============================//
// Methods
//==============================//

protected:

  // Put the received Word into the table, along with its destination address.
  void          receivedData();

  // Stall the pipeline until the channel specified by waitOnChannel is empty.
  void          waitUntilEmpty();

  // Update the value on the stallOut port.
  void          updateStall();

  // If it is possible to send data onto the network, do it. This method is
  // is called at the start of each clock cycle.
  virtual void  canSend();

  // Choose which buffer to put the new data into.
  virtual short chooseBuffer();

//==============================//
// Local state
//==============================//

protected:

  // A buffer for each output channel.
  BufferArray<AddressedWord> buffers;

  // Tells whether or not there is a full buffer, requiring a pipeline stall.
  bool stallValue;

  // Tells whether or not we are currently waiting for a channel to empty.
  bool waiting;

//==============================//
// Signals (wires)
//==============================//

protected:

  // Signal that something has happened which requires the stall output
  // to be changed.
  sc_event stallValueReady;

};

#endif /* SENDCHANNELENDTABLE_H_ */
