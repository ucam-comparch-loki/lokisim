/*
 * ReceiveChannelEndTable.h
 *
 * Component containing a buffer storing data from each input channel.
 *
 * Reads from the table are blocking: if a buffer being read from is empty,
 * the cluster will stall until data has arrived there.
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#ifndef RECEIVECHANNELENDTABLE_H_
#define RECEIVECHANNELENDTABLE_H_

#include "../../../Component.h"
#include "../../../Datatype/Word.h"
#include "../../../Datatype/Address.h"
#include "../../../Datatype/Data.h"
#include "../../../Memory/BufferArray.h"
#include "../../../Utility/LoopCounter.h"

class ReceiveChannelEndTable: public Component {

//==============================//
// Ports
//==============================//

public:

  // Data values received over the network. There should be NUM_RECEIVE_CHANNELS
  // inputs in the array.
  sc_in<Word>  *fromNetwork;

  // A flow control signal for each input (NUM_RECEIVE_CHANNELS), to tell the
  // flow control unit how much space is left in its buffer.
  sc_out<int>  *flowControl;

  // The channels to read data from. fromDecoder1 specifies the channel whose
  // data should be sent to the ALU's first input (and similar for fromDecoder2).
  sc_in<short>  fromDecoder1, fromDecoder2;

  // Data being sent to the ALU.
  sc_out<Data>  toALU1, toALU2;

  // The operation to carry out.
  sc_in<short>  operation;

  // Tell the cluster to stall if it attempts to read from an empty buffer,
  // and allow it to continue again once data has arrived at that buffer.
  sc_out<bool>  stallOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(ReceiveChannelEndTable);
  ReceiveChannelEndTable(sc_module_name name);
  virtual ~ReceiveChannelEndTable();

//==============================//
// Methods
//==============================//

private:

  // Put any newly received values into their respective buffers.
  void receivedInput();

  // Service requests for data when we receive a channel ID to read from.
  void receivedRequest();

  // Return whether or not the channel specified by fromDecoder1 contains data.
  void testChannelEnd();

  // Carry out the operation asked of this component (TSTCH or SELCH). The
  // operation must arrive at the same time as or after the input from
  // fromDecoder1.
  void doOperation();

  // Send new data out on the toALU1 port. There are multiple writers, so this
  // extra method is needed.
  void updateToALU1();

  // Update the flow control values when a buffer has been read from or
  // written to.
  void updateFlowControl();

  // Update the this component's stall output signal.
  void updateStall();

  // Check to see if we were waiting for data on this channel end, and if so,
  // send it immediately. Unstall the pipeline if appropriate.
  void checkWaiting(int channelEnd);

  // Read from the chosen channel end, and write the result to the given output.
  void read(short inChannel, short outChannel);

//==============================//
// Local state
//==============================//

public:

  // The operations the receive channel-end table can carry out.
  enum ChannelOp {TSTCH, SELCH};

private:

  // A buffer for each channel-end.
  BufferArray<Word> buffers;

  // Channel ends we're waiting for data on.
  int               waiting1, waiting2;

  // There are multiple writers to port toALU1, so we need to store the value
  // they want to send.
  Data              dataToALU1;

  // Whether this component is telling the pipeline to stall.
  bool              stallValue;

  // Whether we are waiting to be told which channel-end to perform an
  // operation on.
  bool              waitingForInput;

  // Allows round-robin selection of channels when executing selch.
  LoopCounter       currentChannel;

//==============================//
// Signals (wires)
//==============================//

private:

  // The contents of the buffers have changed, so the flow control signals
  // should be re-evaluated.
  sc_event contentsChanged;

  // Signal that there is new data to be sent on port toALU1.
  sc_event alu1ValueReady;

  // Signal that something has happened which may have changed whether or not
  // this component is causing the pipeline to stall.
  sc_event stallValueReady;

  // Signal that a channel waited on now has data, so a read should take place.
  sc_signal<bool>   endWaiting1, endWaiting2;

};

#endif /* RECEIVECHANNELENDTABLE_H_ */
