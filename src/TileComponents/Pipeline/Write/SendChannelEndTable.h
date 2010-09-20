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
#include "../../../Memory/AddressedStorage.h"
#include "../../../Memory/BufferArray.h"

class AddressedWord;
class DecodedInst;
class Word;

typedef uint8_t ChannelIndex;
typedef uint8_t MapIndex;
typedef uint32_t ChannelID;

class SendChannelEndTable: public Component {

//==============================//
// Ports
//==============================//

public:

  // The outputs to the network. There should be NUM_SEND_CHANNELS of them.
  sc_out<AddressedWord>  *output;

  // A flow control signal for each output (NUM_SEND_CHANNELS), saying
  // whether or not it is allowed to send its next value.
  sc_in<bool>            *flowControl;

//==============================//
// Constructors and destructors
//==============================//

public:

  SendChannelEndTable(sc_module_name name);
  virtual ~SendChannelEndTable();

//==============================//
// Methods
//==============================//

public:

  // Write the result of an operation to one of the output buffers.
  void          write(DecodedInst& dec);

  // Returns true if the table is incapable of accepting new data at the moment.
  bool          isFull() const;

  // Send the oldest value in each output buffer, if the flow control signals
  // allow it.
  void          send();

protected:

  // Stall the pipeline until the channel specified is empty.
  void          waitUntilEmpty(ChannelIndex channel);

  // Update an entry in the channel mapping table.
  void          updateMap(MapIndex entry, ChannelID newVal);

  // Retrieve an entry from the channel mapping table.
  ChannelID     getChannel(MapIndex mapEntry);

  // Choose which buffer to put the new data into (multiple channels may
  // share a buffer to reduce buffer space/energy).
  virtual ChannelIndex chooseBuffer(MapIndex channelMapEntry) const;

  // Generate a memory request using the address from the ALU and the operation
  // supplied by the decoder. The memory request will be sent to a memory and
  // will result in an operation being carried out there.
  Word getMemoryRequest(DecodedInst& dec) const;

//==============================//
// Local state
//==============================//

protected:

  // A buffer for each output channel.
  BufferArray<AddressedWord>  buffers;

  // Channel mapping table used to store addresses of destinations of sent
  // data. Note that mapping 0 is held both here and in the decode stage --
  // it may be possible to optimise this away at some point.
  AddressedStorage<ChannelID> channelMap;

  // Tells which channel we are waiting on. -1 means no channel.
  ChannelIndex                waitingOn;

  // Anything sent to the null channel ID doesn't get sent at all. This allows
  // more code re-use, even in situations where we don't want to send results.
  static const ChannelID      NULL_MAPPING = -1;

  // Used to tell that we are not currently waiting for any output buffers
  // to empty.
  static const ChannelIndex   NO_CHANNEL = -1;

};

#endif /* SENDCHANNELENDTABLE_H_ */
