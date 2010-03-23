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

class ReceiveChannelEndTable: public Component {

//==============================//
// Ports
//==============================//

public:

  // Data values received over the network. There should be NUM_RECEIVE_CHANNELS
  // inputs in the array.
  sc_in<Word>  *fromNetwork;

  // A flow control signal for each input (NUM_RECEIVE_CHANNELS), to stop
  // further values being sent if a buffer is full.
  sc_out<bool> *flowControl;

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

  void receivedInput();
  void read1();
  void read2();
  void testChannelEnd();
  void doOperation();
  void updateToALU1();
  void updateFlowControl();
  void updateStall();
  void checkWaiting(int channelEnd);
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

//==============================//
// Signals (wires)
//==============================//

private:

  // The contents of the buffers have changed, so the flow control signals
  // should be re-evaluated.
  sc_buffer<bool>   readFromBuffer, wroteToBuffer;

  // Signal that there is new data to be sent on port toALU1.
  sc_signal<bool>   updateToALU1_1, updateToALU1_2, updateToALU1_3, updateToALU1_4;

  // Signal that something has happened which may have changed whether or not
  // this component is causing the pipeline to stall.
  sc_signal<bool>   updateStall1, updateStall2, updateStall3;

};

#endif /* RECEIVECHANNELENDTABLE_H_ */
