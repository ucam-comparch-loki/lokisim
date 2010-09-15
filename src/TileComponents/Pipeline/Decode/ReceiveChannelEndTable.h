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
#include "../../../Memory/BufferArray.h"
#include "../../../Utility/LoopCounter.h"

class Word;

typedef uint8_t ChannelIndex;

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

//==============================//
// Constructors and destructors
//==============================//

public:

  ReceiveChannelEndTable(sc_module_name name);
  virtual ~ReceiveChannelEndTable();

//==============================//
// Methods
//==============================//

public:

  // If any data has arrived over the network, put it into the appropriate
  // buffers.
  void checkInputs();

  // Read from the specified channel end.
  int32_t read(ChannelIndex channelEnd);

  // Return whether or not the channel contains data.
  bool testChannelEnd(ChannelIndex channelEnd) const;

  // Return the index of a channel which currently contains data, or throw
  // an exception if none do. The index returned is the channel's register-
  // mapped index (e.g. if channel 0 contains data, 16 would be returned).
  ChannelIndex selectChannelEnd();

private:

  // Update the flow control value for a particular input port.
  void updateFlowControl(uint8_t channelEnd);

//==============================//
// Local state
//==============================//

public:

  // The operations the receive channel-end table can carry out.
  enum ChannelOp {TSTCH, SELCH};

private:

  // A buffer for each channel-end.
  BufferArray<Word> buffers;

  // Allows round-robin selection of channels when executing selch.
  LoopCounter       currentChannel;

};

#endif /* RECEIVECHANNELENDTABLE_H_ */
