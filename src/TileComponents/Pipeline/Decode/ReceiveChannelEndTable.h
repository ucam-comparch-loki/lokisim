/*
 * ReceiveChannelEndTable.h
 *
 * Component containing a vector of buffers storing data from input channels.
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
  void updateFlowControl();
  void updateStall();
  void checkWaiting(int channelEnd);
  void read(short inChannel, short outChannel);

//==============================//
// Local state
//==============================//

private:

  BufferArray<Word> buffers;
  int               waiting1, waiting2; // Channel ends we're waiting for data on
  bool              stallValue;

//==============================//
// Signals (wires)
//==============================//

private:

  sc_buffer<bool>   readFromBuffer, wroteToBuffer;
  sc_signal<bool>   updateStall1, updateStall2;

};

#endif /* RECEIVECHANNELENDTABLE_H_ */
