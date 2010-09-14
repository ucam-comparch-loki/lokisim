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
//#include "../../../Datatype/Word.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/AddressedWord.h"
#include "../../../Memory/AddressedStorage.h"
#include "../../../Memory/BufferArray.h"

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
  bool          isFull();

  // Send the oldest value in each output buffer, if the flow control signals
  // allow it.
  void          send();

protected:

  // Stall the pipeline until the channel specified is empty.
  void          waitUntilEmpty(uint8_t channel);

  // Update an entry in the channel mapping table.
  void          updateMap(uint8_t entry, uint32_t newVal);

  uint32_t      getChannel(uint8_t mapEntry);

  // Choose which buffer to put the new data into (multiple channels may
  // share a buffer to reduce buffer space/energy).
  virtual uint8_t chooseBuffer(uint8_t channelMapEntry);

//==============================//
// Local state
//==============================//

protected:

  // A buffer for each output channel.
  BufferArray<AddressedWord> buffers;

  // Channel mapping table used to store addresses of destinations of sent
  // data.
  AddressedStorage<uint32_t> channelMap;

  // Tells whether or not there is a full buffer, requiring a pipeline stall.
  bool stallValue;

  // Tells which channel we are waiting on. 255 means no channel.
  uint8_t waitingOn;

  // Signal that something has happened which requires the stall output
  // to be changed.
  sc_event stallValueReady;

  static const uint32_t NULL_MAPPING = -1;

};

#endif /* SENDCHANNELENDTABLE_H_ */
